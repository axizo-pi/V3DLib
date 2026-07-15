#include "global/log.h"
#include "global.h"
#include <iostream>

using namespace std;
using namespace Log;

namespace {

/**
 * TODO: Perhaps convert to MMatrix
 */
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
  init_matrix(tmp, input_dim, hidden_dim);

  U_z.set(tmp);
  U_r.set(tmp);
  U_h.set(tmp);

  init_matrix(tmp, hidden_dim, hidden_dim);
  W_z.set(tmp);
  W_r.set(tmp);
  W_h.set(tmp);
  
  init_matrix(tmp, hidden_dim, output_dim);
  V.set(tmp);

  eval();
}


void Model::init_val(int input_dim, int hidden_dim, int output_dim, float val, bool do_eval) {
  assert(val == 0.0f || val == 1.0f);

  //warn << "U_z: " << U_z.dump_dim();
  if (!U_z.empty()) {
    assert(U_z.rows() == input_dim);
    assert(U_z.cols() == hidden_dim);
    // Assuming all other matrices are okay 

    // 2x faster than full resize
    timers.start("init_val set");

    U_z.set(val);
    U_r.set(val);
    U_h.set(val);

    W_z.set(val);
    W_r.set(val);
    W_h.set(val);

    V.set(val);

    timers.stop("init_val set");
  } else {
    timers.start("init_val resize");

    U_z.resize(input_dim, hidden_dim, val);
    U_r.resize(input_dim, hidden_dim, val);
    U_h.resize(input_dim, hidden_dim, val);

    W_z.resize(hidden_dim, hidden_dim, val);
    W_r.resize(hidden_dim, hidden_dim, val);
    W_h.resize(hidden_dim, hidden_dim, val);

    V.resize(hidden_dim, output_dim, val);

    timers.stop("init_val resize");
  }

  if (do_eval) {
    eval();
  }
}


/**
 * This method specifically to test if gradient changed
 */
bool Model::is_zero() const {
  //timers.start("Model::is_zero()");

  bool ret =
    U_z.is_zero() &&
    U_r.is_zero() &&
    U_h.is_zero() &&
    W_z.is_zero() &&
    W_r.is_zero() &&
    W_h.is_zero() &&
    V.is_zero();

  //timers.stop("Model::is_zero()");
  return ret;
}


void Model::grad_div_steps(int in_steps) {
  assert(!is_zero());
  if (is_zero()) return;
  //warn << "Doing grad_div_steps, steps: " << in_steps;

  float steps = (float) in_steps;

  U_z /= steps;     // OK

  U_r /= steps;     // OK

  U_h /= steps;
  W_z /= steps;
  W_r /= steps;

  //auto prev = W_h;
  W_h /= steps;     // OK
  //W_h.diff(prev);

  //warn << "grad_div_steps V pre: " << V.dump();
  V   /= steps;
  assert(V.is_zero());  // Zero on init, warn me if it changes

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


MMatrix &State::S()             { assert(!m_S.empty()); return m_S; }
MMatrix const &State::S() const { assert(!m_S.empty()); return m_S; }


/**
 * Timing inconsequential
 */
void State::init(int time_steps, int hidden_dim, int output_dim) {
  assert(z.empty());  // Call only once

  z.resize(time_steps, hidden_dim);
  r.resize(time_steps, hidden_dim);
  h.resize(time_steps, hidden_dim);

  if (m_do_temp) {
    m_S.resize(time_steps, hidden_dim);
  } else {
    E.resize(1, time_steps);
    O.resize(time_steps, output_dim);

    MatrixXf tmp_S = MatrixXf::Zero(time_steps + 1, hidden_dim);
    tmp_S(0, 0) = static_cast <float> (((float) rand()) / (static_cast <float> (RAND_MAX / 2)) - 1);
    //tmp_S.eval();
    m_S.set(tmp_S, true);
  }
}


void State::eval() {
  E.eval();
  z.eval();
  r.eval();
  h.eval();
  O.eval();
  m_S.eval();
}


void State::set_step(int time_step, State const &state) {
  assert(m_do_temp);

  m_S.set(state.m_S.row(time_step));
  r.set(state.r.row(time_step));
  z.set(state.z.row(time_step));
  h.set(state.h.row(time_step));
}


void State::move_rows(int step, State const &state) {
  assert(m_do_temp);

  m_S.move_rows(step, state.m_S);
  r.move_rows(step, state.r);
  z.move_rows(step, state.z);
  h.move_rows(step, state.h);
}


