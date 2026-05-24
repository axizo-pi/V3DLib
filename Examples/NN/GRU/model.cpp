#include "global/log.h"
#include "global.h"
#include "Support/Helpers.h"  // bit_diff()
#include <iostream>

using namespace std;
using namespace Log;

namespace {

float s_max = 0;

float softmax(float x) {
  return (float) exp(x - s_max);
}


void init_matrix(MatrixXf& X, int dimension_row, int dimension_col) {
  float upperlimit =  1.0f * (float) sqrt(1.0f / (float) dimension_row);
  float lowerlimit = -1.0f * (float) sqrt(1.0f / (float) dimension_row);
  float range = upperlimit - lowerlimit;

  srand((unsigned) clock());
  X = MatrixXf::Random(dimension_row, dimension_col);
  X = (X + MatrixXf::Constant(dimension_row, dimension_col, 1.0)) * (range / 2.0);
  X = (X + MatrixXf::Constant(dimension_row, dimension_col, lowerlimit));
}

} // anon namespace


void Model::read(string const &epoch, string const &loss) {
  string postfix = "_epoch_" + epoch + "_loss_" + loss + ".bin";
  MatrixXf tmp;

  read_binary_matrix("Weights/Uz" + postfix, tmp); U_z.set(tmp);
  read_binary_matrix("Weights/Uh" + postfix, tmp); U_h.set(tmp);
  read_binary_matrix("Weights/Ur" + postfix, tmp); U_r.set(tmp);
  read_binary_matrix("Weights/Wz" + postfix, tmp); W_z.set(tmp);
  read_binary_matrix("Weights/Wh" + postfix, tmp); W_h.set(tmp);
  read_binary_matrix("Weights/Wr" + postfix, tmp); W_r.set(tmp);
  read_binary_matrix("Weights/V"  + postfix, tmp);   V.set(tmp);
}


void Model::write(int epoch, float loss) {
  std::cout << "Writing weights to file. " << std::endl;

  string postfix = "_epoch_" + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin";

  write_binary_matrix("Weights/Uz" + postfix, U_z.Xf());
  write_binary_matrix("Weights/Uh" + postfix, U_h.Xf());
  write_binary_matrix("Weights/Ur" + postfix, U_r.Xf());
  write_binary_matrix("Weights/Wz" + postfix, W_z.Xf());
  write_binary_matrix("Weights/Wh" + postfix, W_h.Xf());
  write_binary_matrix("Weights/Wr" + postfix, W_r.Xf());
  write_binary_matrix("Weights/V"  + postfix,   V.Xf());
}


std::string Model::dump_dim() const {
  std::string ret;

  ret << "(input, hidden, output): ("
      << input_dim()  << ", "
      << hidden_dim() << ", "
      << output_dim() << ")";

  return ret;
}  


void Model::init(int input_dim, int hidden_dim, int output_dim) {
  MatrixXf tmp;

  init_matrix(tmp, input_dim, hidden_dim); U_z.set(tmp);
  init_matrix(tmp, input_dim, hidden_dim); U_r.set(tmp);
  init_matrix(tmp, input_dim, hidden_dim); U_h.set(tmp);

  init_matrix(tmp, hidden_dim, hidden_dim); W_z.set(tmp);
  init_matrix(tmp, hidden_dim, hidden_dim); W_r.set(tmp);
  init_matrix(tmp, hidden_dim, hidden_dim); W_h.set(tmp);
  
  init_matrix(tmp, hidden_dim, output_dim);   V.set(tmp);

  eval();
}


void Model::init_zeroes(int input_dim, int hidden_dim, int output_dim, bool do_eval) {
  auto zero   = MatrixXf::Zero(input_dim, hidden_dim);
  auto zero_h = MatrixXf::Zero(hidden_dim, hidden_dim);
  auto zero_o = MatrixXf::Zero(hidden_dim, output_dim);

  U_z.set(zero);
  U_r.set(zero);
  U_h.set(zero);

  W_z.set(zero_h);
  W_r.set(zero_h);
  W_h.set(zero_h);

  V.set(zero_o);

  if (do_eval) {
    eval();
  }
}


void Model::init_ones(int input_dim, int hidden_dim, int output_dim) {
  auto ones   = MatrixXf::Ones(input_dim, hidden_dim);
  auto ones_h = MatrixXf::Ones(hidden_dim, hidden_dim);
  auto ones_o = MatrixXf::Ones(hidden_dim, output_dim);

  U_z.set(ones);
  U_r.set(ones);
  U_h.set(ones);

  W_z.set(ones_h);
  W_r.set(ones_h);
  W_h.set(ones_h);

  V.set(ones_o);

  eval();
}


void Model::grad_div_steps(float steps) {
  U_z /= steps;
  U_r /= steps;
  U_h /= steps;
  W_z /= steps;
  W_r /= steps;
  W_h /= steps;
  V   /= steps;

  eval();
}


void Model::cache_decay(float decay, Model &grad) {
  //timers.start("cache_decay");

  U_z.set_decay(decay, grad.U_z);
  U_r.set_decay(decay, grad.U_r);
  U_h.set_decay(decay, grad.U_h);
  W_z.set_decay(decay, grad.W_z);
  W_r.set_decay(decay, grad.W_r);
  W_h.set_decay(decay, grad.W_h);
    V.set_decay(decay, grad.V  );

  eval();

  //timers.stop("cache_decay");
}


void Model::divide(Model &grad, Model &cache) {
  //timers.start("divide");

  U_z.divide_matrix(grad.U_z, cache.U_z);
  U_r.divide_matrix(grad.U_r, cache.U_r);
  U_h.divide_matrix(grad.U_h, cache.U_h);
  W_z.divide_matrix(grad.W_z, cache.W_z);
  W_r.divide_matrix(grad.W_r, cache.W_r);
  W_h.divide_matrix(grad.W_h, cache.W_h);
    V.divide_matrix(grad.V  , cache.V  );

  eval();

  //timers.stop("divide");
}


void Model::adjust_learning_rate(float learning_rate, Model &rhs) {
  //timers.start("adjust_learning_rate");

  U_z -= learning_rate * rhs.U_z;
  U_r -= learning_rate * rhs.U_r;
  U_h -= learning_rate * rhs.U_h;
  W_z -= learning_rate * rhs.W_z;
  W_r -= learning_rate * rhs.W_r;
  W_h -= learning_rate * rhs.W_h;
    V -= learning_rate * rhs.V;

  //timers.stop("adjust_learning_rate");
}


void Model::eval() {
  U_z.eval();
  U_r.eval();
  U_h.eval();
  W_z.eval();
  W_r.eval();
  W_h.eval();
  V.eval();
}


void State::init(int time_steps, int hidden_dim, int output_dim) {
  //warn << "State::init()";

  if (m_do_temp) {
    auto zero = MatrixXf::Zero(time_steps, hidden_dim);

    S.set(zero);
    z.set(zero);
    r.set(zero);
    h.set(zero);

  } else {
    //auto zero   = MatrixXf::Zero(resize_16(time_steps), hidden_dim);
    auto zero   = MatrixXf::Zero(time_steps, hidden_dim);
    auto zero_1 = MatrixXf::Zero(1, time_steps);
    auto zero_o = MatrixXf::Zero(time_steps, output_dim);

    E.set(zero_1);
    z.set(zero);
    r.set(zero);
    h.set(zero);
    O.set(zero_o);

    //MatrixXf tmp_S = MatrixXf::Zero(resize_16(time_steps + 1), hidden_dim);
    MatrixXf tmp_S = MatrixXf::Zero(time_steps + 1, hidden_dim);
    tmp_S(0, 0) = static_cast <float> (((float) rand()) / (static_cast <float> (RAND_MAX / 2)) - 1);
    //tmp_S.eval();

    S.set(tmp_S);
    // OK warn << "State::init S: " << S.dump();
  }
}


void State::eval() {
  E.eval();
  z.eval();
  r.eval();
  h.eval();
  O.eval();
  S.eval();
}


void State::set_step(int time_step, State const &state) {
  assert(m_do_temp);

  S.set(state.S.row(time_step));
  r.set(state.r.row(time_step));
  z.set(state.z.row(time_step));
  h.set(state.h.row(time_step));
}


void State::move_rows(int step, State const &state) {
  assert(m_do_temp);

  S.move_rows(step, state.S);
  r.move_rows(step, state.r);
  z.move_rows(step, state.z);
  h.move_rows(step, state.h);
}


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

  MatrixXf temp        = MatrixXf::Zero(1, hidden_dim);

  MatrixXf temp_output = MatrixXf::Zero(1, output_dim);
  MatrixXf temp_hidden = MatrixXf::Zero(1, hidden_dim);
  MatrixXf ones        = MatrixXf::Ones(1, hidden_dim);

  //warn << "state.S: " << state.S.dump_dim();
  //warn << "m.W_z: " << m.W_z.dump_dim();

  ////// QPU /////

  MMatrix x_ones(time_steps, hidden_dim, 1.0f);
  MMatrix x_X;
  x_X.set(X);
  MMatrix x_Y;
  x_Y.set(Y);
  MMatrix x_z_in;
  x_z_in.set(state.z);

  State x_state = state;
  x_state.S     = remove_last_rows(1, x_state.S);
  MMatrix x_S_extra(1, hidden_dim);
  //warn << "x_S_extra: " << x_S_extra.dump();

  timers.start("forward temp2");

  MMatrix temp2 = (x_X * m.U_z) + (x_state.S*m.W_z);
  x_state.z = temp2.sigmoid();

  MMatrix temp3 = (x_X * m.U_r) + (x_state.S * m.W_r);
  x_state.r = temp3.sigmoid();

  MMatrix temp4 = (x_X * m.U_h) + (x_state.S.mul_e(x_state.r) * m.W_h);

  x_state.h = temp4.tanh();

  MMatrix x_temp_hidden = (x_ones - x_z_in).mul_e(x_state.h + x_z_in).mul_e(x_state.S);

  x_S_extra = x_temp_hidden.row(time_steps - 1);
  x_state.S.move_rows(1, x_temp_hidden);

  MMatrix x_temp_output = x_temp_hidden * m.V;

  MMatrix x_s_max = x_temp_output.max_row();
  //warn << "x_s_max: " << x_s_max.dump();

  x_temp_output.softmax();

  x_state.O = x_temp_output.div_e(x_temp_output.sum_row());

  timers.stop("forward temp2");

  ////// End QPU /////

  for(int i = 0; i < time_steps; i++) {
    warn << "Forward i: " << i;

    auto S_row = state.S.row(i);
    //warn << "S_row: " << S_row.dump_dim();
    MMatrix X_row;
    X_row.set(X.row(i));
    auto z_row = state.z.row(i);

    timers.start("forward temp");
    temp = (X_row.Xf() * (m.U_z.Xf())) + (S_row.Xf() * (m.W_z.Xf()));
    temp.eval();
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
    float x_temp_sum = x_temp_output.sum_row().qpu().at(i, 0);
    //warn << "temp_output.sum(): " << temp_output.sum();
    //warn << "x_temp_output.row_sum(), row(i): " << x_temp_output.sum_row().qpu().at(i, 0);
    //warn << "bit_diff: " << bit_diff(temp_sum, x_temp_sum, -1);

    int diff = bit_diff(temp_sum, x_temp_sum, 3);  // bit index kind of high, usually lower
		if (diff != -1) {
			warn << "bit_diff: " << diff;
			assert(false);
		}

    state.O.row(i, temp_output / temp_output.sum());
    state.O.eval();
    assert(x_state.O.row(i).same(state.O.row(i), Precision));

    //==================================

    if (!do_test) {
			//warn << "state.E: " << state.E.dump_dim();
      state.E.update_E(i, x_Y.row(i), state.O.row(i));
    }
  }

  timers.stop("forward_propagation");
}
