#include "forward.h"
#include "global.h"
#include "Support/Helpers.h"  // bit_diff()

namespace {

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
	x_temp_hidden = x_z_in.forward_4(x_state.S, x_state.h);

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
 *
 * - steps z_t, r_t do NOT use ReLU, but sigmoid()
 * - Step h_t uses tanh(), not ReLU
 * - Step S_t tanh() missing entirely
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

  MMatrix temp(1, hidden_dim);
  MatrixXf temp_hidden = MatrixXf::Zero(1, hidden_dim);

  ////// QPU /////

  MMatrix x_X;
  x_X.set(X);
  MMatrix x_Y;
  x_Y.set(Y);

	MMatrix temp2 = x_X.forward_1(m, x_state.S);
  x_state.z = temp2.sigmoid();

	MMatrix temp3 = x_X.forward_2(m, x_state.S);
  x_state.r = temp3.sigmoid();

	MMatrix temp4 = x_X.forward_3(m, x_state.S, x_state.r);
  x_state.h = temp4.tanh();

  update_S_O(time_steps, m);

  if (!do_test) {
    MMatrix E_ret = x_state.E.calc_E(x_Y, x_state.O).transpose();
    x_state.E += E_ret;
    //warn << "E_ret: " << E_ret.dump();
    //warn << "x_state.E: " << x_state.E.dump_dim();
    assert(x_state.E.same());
  }

  ////// End QPU /////

  for(int i = 0; i < time_steps; i++) {
    warn << "Forward i: " << i;

    auto S_row = state.S.row(i);
    MMatrix X_row;
    X_row.set(X.row(i));
    auto z_row = state.z.row(i);

		temp = X_row.forward_1(m, S_row);
    assert(temp2.row(i).same(temp));

    state.z.row(i, temp.sigmoid());
    assert(x_state.z.row(i).same_b(state.z.row(i), -1));

		temp = X_row.forward_2(m, S_row);
    assert(temp3.row(i).same(temp));
/*
		warn << "Here1";
    MatrixXf sig1 = temp.Xf().unaryExpr(&sigmoid);
    auto sig2 = temp.sigmoid();
    assert(sig2.same_b(sig1, 1));  // Usually bit <= 1, sometimes bit == 3
*/
    state.r.row(i, temp.sigmoid());
    assert(x_state.r.row(i).same_b(state.r.row(i), -1));

		MMatrix tempb = X_row.forward_3(m, S_row, state.r.row(i));
    state.h.row(i, tempb.tanh());

		MMatrix temp_hidden = z_row.forward_4(S_row, state.h.row(i));

    assert(temp4.row(i).same(tempb, Precision));
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

		auto temp_output = state.S.row(i + 1) * m.V;

    temp_output.softmax();
    assert(x_temp_output.row(i).same(temp_output));

    float temp_sum = temp_output.Xf().sum();
    check_sum(i, temp_sum);

    state.O.row(i, temp_output/temp_sum);
    state.O.eval();
    assert(x_state.O.row(i).same(state.O.row(i), Precision));
		warn << "Here done";

    if (!do_test) {
      MMatrix ret = state.E.calc_E(x_Y.row(i), state.O.row(i));
      //warn << "E ret: " << ret.dump();
      //warn << "state.E pre: " << state.E.dump();

      state.E.col_E(i, ret);
      //warn << "state.E: " << state.E.dump();
      float diff = abs(x_state.E.Xf()(0, i) - state.E.Xf()(0, i));
      //warn << "diff: " << diff;
      assert(diff < Precision);
    }
  }

  timers.stop("forward_propagation");
}
