#include "forward.h"
#include "global.h"
#include "Support/Helpers.h"  // bit_diff()

namespace {

float s_max = 0;

float softmax(float x) {
  return (float) exp(x - s_max);
}

MatrixXf ones;
MMatrix  x_ones;

State    x_state;
MMatrix  x_S_extra;
MMatrix  x_temp_hidden;
MMatrix  x_temp_output;
MMatrix  x_z_in;


void init(
  State &state,
  int time_steps,
  int input_dim,
  int hidden_dim,
  int output_dim
) {
  x_ones.resize(time_steps, hidden_dim, 1.0f);
  ones = MatrixXf::Ones(1, hidden_dim);
  //warn << "init x_ones: " << x_ones.dump_dim();
  //warn << "init ones: " << dump(ones);

  x_z_in.set(state.z);

  x_state   = state;
  x_state.S = remove_last_rows(1, x_state.S);
  x_S_extra.resize(1, hidden_dim);
  //warn << "init x_state.S: " << x_state.S.dump_dim();
  //warn << "init x_S_extra: " << x_S_extra.dump();
}


/**
 * NOTE: dependency on x_state.h
 */
void update_S_O(int time_steps, Model &m) {
  x_temp_hidden = (x_ones - x_z_in).mul_e(x_state.h + x_z_in).mul_e(x_state.S);
  x_S_extra = x_temp_hidden.row(time_steps - 1);

  x_state.S.move_rows(1, x_temp_hidden);

  x_temp_output = x_temp_hidden * m.V;
  x_temp_output.softmax();
  x_state.O = x_temp_output.div_e(x_temp_output.sum_row());
}


void check_sum(int i, float temp_sum) {
  float x_temp_sum = x_temp_output.sum_row().qpu().at(i, 0);
  //warn << "temp_output.sum(): " << temp_output.sum();
  //warn << "x_temp_output.row_sum(), row(i): " << x_temp_output.sum_row().qpu().at(i, 0);
  //warn << "bit_diff: " << bit_diff(temp_sum, x_temp_sum, -1);

  int diff = bit_diff(temp_sum, x_temp_sum, 3);  // bit index kind of high, usually lower
  if (diff != -1) {
    warn << "bit_diff: " << diff;
    assert(false);
  }
}

} // anon namespace

/**
 * =============================
 * Notes
 * -----
 *
 * - Forward propagation Step - returns z, r, h, S, O, E
 *
 *     z_t = ReLU[(X_t * U_z) + (S_t-1 * W_z)]
 *     r_t = ReLU[(X_t * U_r) + (S_t-1 * W_r)]
 *     h_t = ReLU[(X_t * U_h) + ((S_t-1 o r_t) * W_h)]
 *     S_t = tanh[((1 - z_t) o h_t) + (z_t o S_t-1)]
 *     O   = softmax(S_t * V)
 *     E   = - 1/N sum(Y o log(O))
 *
 * where * is matrix multiplication and o is componentwise multiplication
 */
void forward_propagation(
  Model &m,
  MatrixXf& X,
  MatrixXf& Y,  // Not used in test
  State &state,
  int time_steps,
  int input_dim,
  int hidden_dim,
  int output_dim,
  bool do_test
) {
  timers.start("forward_propagation");
  init(state, time_steps, input_dim, hidden_dim, output_dim);

  MatrixXf temp        = MatrixXf::Zero(1, hidden_dim);
  MatrixXf temp_output = MatrixXf::Zero(1, output_dim);
  MatrixXf temp_hidden = MatrixXf::Zero(1, hidden_dim);

  ////// QPU /////

  MMatrix x_X;
  x_X.set(X);
  MMatrix x_Y;
  x_Y.set(Y);
  timers.start("forward temp2");

  MMatrix temp2 = (x_X * m.U_z) + (x_state.S*m.W_z);
  x_state.z = temp2.sigmoid();

  MMatrix temp3 = (x_X * m.U_r) + (x_state.S * m.W_r);
  x_state.r = temp3.sigmoid();

  MMatrix temp4 = (x_X * m.U_h) + (x_state.S.mul_e(x_state.r) * m.W_h);
  x_state.h = temp4.tanh();

  update_S_O(time_steps, m);

  if (!do_test) {
    MMatrix E_ret = x_state.E.calc_E(x_Y, x_state.O).transpose();
    x_state.E += E_ret;
    //warn << "E_ret: " << E_ret.dump();
    //warn << "x_state.E: " << x_state.E.dump_dim();
    assert(x_state.E.same());
  }

  timers.stop("forward temp2");

  ////// End QPU /////

  for(int i = 0; i < time_steps; i++) {
    warn << "Forward i: " << i;

    auto S_row = state.S.row(i);
    MMatrix X_row;
    X_row.set(X.row(i));
    auto z_row = state.z.row(i);

    timers.start("forward temp");
    temp = (X_row.Xf() * (m.U_z.Xf())) + (S_row.Xf() * (m.W_z.Xf()));
    temp.eval();
    warn << "temp: " << dump(temp);
    warn << "temp2(" << i << "): " << temp2.row(i).dump();
    timers.stop("forward temp");
    assert(::same(temp2.row(i).qpu(), temp));

    state.z.row(i, temp.unaryExpr(&sigmoid));
    assert(x_state.z.row(i).same(state.z.row(i), 4*Precision));  // TODO check precision when forward prop done

    timers.start("forward temp");
    temp = (X_row.Xf() * (m.U_r.Xf())) + (S_row.Xf() * (m.W_r.Xf()));
    temp.eval();
    timers.stop("forward temp");
    assert(::same(temp3.row(i).qpu(), temp));

    state.r.row(i, temp.unaryExpr(&sigmoid));
    assert(x_state.r.row(i).same(state.r.row(i), 4*Precision));  // TODO check precision when forward prop done

    timers.start("forward temp");
    temp = (X_row.Xf() * (m.U_h.Xf())) + (S_row.Xf().cwiseProduct(state.r.row(i).Xf())) * (m.W_h.Xf());
    temp.eval();

    state.h.row(i, temp.unaryExpr(&tanh_activation));

    temp_hidden     = (ones - z_row.Xf()).cwiseProduct(state.h.row(i).Xf() + z_row.Xf()).cwiseProduct(S_row.Xf());
    temp_hidden.eval();

    timers.stop("forward temp");

    assert(::same(temp4.row(i).qpu(), temp, Precision));
    assert(x_state.h.row(i).same(state.h.row(i), Precision));
    assert(x_temp_hidden.row(i).same(temp_hidden, Precision));

    // Assumption: value at i == 0 should be retained
    state.S.row(i + 1, temp_hidden);
    if (i == time_steps - 1) {
      assert(x_S_extra.same(state.S.row(i)));
    } else {
      //warn << "state.S.row(" << (i + 1) << "): " << state.S.row(i + 1).dump();
      //warn << "x_state.S.row(" << (i + 1) << "): " << x_state.S.row(i + 1).dump();
      assert(x_state.S.row(i + 1).same(state.S.row(i + 1), Precision));
    }

    /// Should be able to use temp_hidden directly, instead of row(i+1)
    assert(state.S.row(i + 1).same(temp_hidden));

    temp_output   = state.S.row(i + 1).Xf() * (m.V.Xf());
    temp_output.eval();
    //assert(x_temp_output.row(i).same(temp_output, Precision));

    // s_max is a global used in softmax()
    s_max          = temp_output.maxCoeff();

    temp_output    = temp_output.unaryExpr(&softmax);
    temp_output.eval();
    //warn << "temp_output: " << dump(temp_output);
    //warn << "x_temp_output.row(i): " << x_temp_output.row(i).dump();
    assert(x_temp_output.row(i).same(temp_output, Precision));

    float temp_sum = temp_output.sum();
    check_sum(i, temp_sum);

    state.O.row(i, temp_output/temp_sum);
    state.O.eval();
    assert(x_state.O.row(i).same(state.O.row(i), Precision));

    if (!do_test) {
      MMatrix ret = state.E.calc_E(x_Y.row(i), state.O.row(i));
      //warn << "E ret: " << ret.dump();
      //warn << "state.E pre: " << state.E.dump();

      state.E.col_E(i, ret);
      warn << "state.E: " << state.E.dump();
      float diff = abs(x_state.E.Xf()(0, i) - state.E.Xf()(0, i));
      warn << "diff: " << diff;
      assert(diff < Precision);
    }
  }

  timers.stop("forward_propagation");
}
