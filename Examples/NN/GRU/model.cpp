#include "model.h"
#include "global/log.h"
#include "helpers.h"  // resize_16()
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

 	S  = ::move_rows(step, state.S);
  r  = ::move_rows(step, state.r);
  z  = ::move_rows(step, state.z);
  h  = ::move_rows(step, state.h);
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

  for(int i = 0; i < time_steps; i++) {
    auto S_row = state.S.row(i).Xf();
    auto X_row = X.row(i);
    auto z_row = state.z.row(i).Xf();

    temp            = (X_row * (m.U_z.Xf())) + (S_row * (m.W_z.Xf()));
    temp.eval();
    state.z.row(i, temp.unaryExpr(&sigmoid));

    temp            = (X_row * (m.U_r.Xf())) + (S_row * (m.W_r.Xf()));
    temp.eval();
    state.r.row(i, temp.unaryExpr(&sigmoid));

    temp            = (X_row * (m.U_h.Xf())) + (S_row.cwiseProduct(state.r.row(i).Xf())) * (m.W_h.Xf());
    temp.eval();
    state.h.row(i, temp.unaryExpr(&tanh_activation));

    temp_hidden     = (ones - z_row).cwiseProduct(state.h.row(i).Xf() + z_row).cwiseProduct(S_row);
    temp_hidden.eval();
    state.S.row(i + 1, temp_hidden);

    temp_output   = state.S.row(i + 1).Xf() * (m.V.Xf());
    temp_output.eval();

    s_max          = temp_output.maxCoeff();
    temp_output    = temp_output.unaryExpr(&softmax);
    temp_output.eval();

    state.O.row(i, temp_output / temp_output.sum());
    state.O.eval();

    if (!do_test) {
      state.E.update_E(i, Y, state);
    }
  }

  timers.stop("forward_propagation");
}
