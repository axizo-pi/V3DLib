#include "model.h"
#include "global/log.h"
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


void divide_matrix(MatrixXf& gradient_total, MatrixXf gradient, MatrixXf cache) {
  for (int i = 0; i < gradient_total.rows(); ++i) {
    for (int j = 0; j < gradient_total.cols(); ++j) {
      gradient_total(i, j) = ( gradient(i, j) / ( sqrt(cache(i, j)) + 0.00000001f) );
    }
  }
}

} // anon namespace


void Model::read(string const &epoch, string const &loss) {
  string postfix = "_epoch_" + epoch + "_loss_" + loss + ".bin";

  read_binary_matrix("Weights/Uz" + postfix, U_z);
  read_binary_matrix("Weights/Uh" + postfix, U_h);
  read_binary_matrix("Weights/Ur" + postfix, U_r);
  read_binary_matrix("Weights/Wz" + postfix, W_z);
  read_binary_matrix("Weights/Wh" + postfix, W_h);
  read_binary_matrix("Weights/Wr" + postfix, W_r);
  read_binary_matrix("Weights/V"  + postfix, V);
}


void Model::write(int epoch, float loss) {
  std::cout << "Writing weights to file. " << std::endl;

	string postfix = "_epoch_" + std::to_string(epoch) + "_loss_" + std::to_string(loss) + ".bin";

  write_binary_matrix("Weights/Uz" + postfix, U_z);
  write_binary_matrix("Weights/Uh" + postfix, U_h);
  write_binary_matrix("Weights/Ur" + postfix, U_r);
  write_binary_matrix("Weights/Wz" + postfix, W_z);
  write_binary_matrix("Weights/Wh" + postfix, W_h);
  write_binary_matrix("Weights/Wr" + postfix, W_r);
  write_binary_matrix("Weights/V"  + postfix, V);
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

  init_matrix(U_z, input_dim, hidden_dim);
  init_matrix(U_r, input_dim, hidden_dim);
  init_matrix(U_h, input_dim, hidden_dim);
  init_matrix(W_z, hidden_dim, hidden_dim);
  init_matrix(W_r, hidden_dim, hidden_dim);
  init_matrix(W_h, hidden_dim, hidden_dim);
  init_matrix(V, hidden_dim, output_dim);

	eval();
	warn << "Done Model::init()\n" << dump_dim();
}


void Model::init_zeroes(int input_dim, int hidden_dim, int output_dim, bool do_eval) {
  U_z = MatrixXf::Zero(input_dim, hidden_dim);
  U_r = MatrixXf::Zero(input_dim, hidden_dim);
  U_h = MatrixXf::Zero(input_dim, hidden_dim);
  W_z = MatrixXf::Zero(hidden_dim, hidden_dim);
  W_r = MatrixXf::Zero(hidden_dim, hidden_dim);
  W_h = MatrixXf::Zero(hidden_dim, hidden_dim);
  V   = MatrixXf::Zero(hidden_dim, output_dim);

	if (do_eval) {
		eval();
	}
}


void Model::init_ones(int input_dim, int hidden_dim, int output_dim) {
  U_z = MatrixXf::Ones(input_dim, hidden_dim);
  U_r = MatrixXf::Ones(input_dim, hidden_dim);
  U_h = MatrixXf::Ones(input_dim, hidden_dim);
  W_z = MatrixXf::Ones(hidden_dim, hidden_dim);
  W_r = MatrixXf::Ones(hidden_dim, hidden_dim);
  W_h = MatrixXf::Ones(hidden_dim, hidden_dim);
  V   = MatrixXf::Ones(hidden_dim, output_dim);

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
	//warn << "grad:" << grad.dump_dim();
	//warn << "U_z :" << ::dump_dim(U_z);

  U_z = decay * U_z + (1 - decay) * (grad.U_z.cwiseProduct(grad.U_z)).eval();
  U_r = decay * U_r + (1 - decay) * (grad.U_r.cwiseProduct(grad.U_r)).eval();
  U_h = decay * U_h + (1 - decay) * (grad.U_h.cwiseProduct(grad.U_h)).eval();
  W_z = decay * W_z + (1 - decay) * (grad.W_z.cwiseProduct(grad.W_z)).eval();
  W_r = decay * W_r + (1 - decay) * (grad.W_r.cwiseProduct(grad.W_r)).eval();
  W_h = decay * W_h + (1 - decay) * (grad.W_h.cwiseProduct(grad.W_h)).eval();
  V   = decay * V   + (1 - decay) * (grad.V  .cwiseProduct(grad.V  )).eval();

  eval();
}


void Model::divide(Model &grad, Model &cache) {
  divide_matrix(U_z, grad.U_z, cache.U_z);
  divide_matrix(U_r, grad.U_r, cache.U_r);
  divide_matrix(U_h, grad.U_h, cache.U_h);
  divide_matrix(W_z, grad.W_z, cache.W_z);
  divide_matrix(W_r, grad.W_r, cache.W_r);
  divide_matrix(W_h, grad.W_h, cache.W_h);
  divide_matrix(V  , grad.V  , cache.V);

  eval();
}


void Model::adjust_learning_rate(float learning_rate, Model &rhs) {
  U_z -= learning_rate * rhs.U_z;
  U_r -= learning_rate * rhs.U_r;
  U_h -= learning_rate * rhs.U_h;
  W_z -= learning_rate * rhs.W_z;
  W_r -= learning_rate * rhs.W_r;
  W_h -= learning_rate * rhs.W_h;
  V   -= learning_rate * rhs.V;
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
	if (m_do_temp) {
		assert(time_steps == 1);

    S = MatrixXf::Zero(1, hidden_dim);
    z = MatrixXf::Zero(1, hidden_dim);
    r = MatrixXf::Zero(1, hidden_dim);
    h = MatrixXf::Zero(1, hidden_dim);

	} else {
	  E = MatrixXf::Zero(1, time_steps);
	  z = MatrixXf::Zero(time_steps, hidden_dim);
	  r = MatrixXf::Zero(time_steps, hidden_dim);
  	h = MatrixXf::Zero(time_steps, hidden_dim);
	  O = MatrixXf::Zero(time_steps, output_dim);
	  S = MatrixXf::Zero(time_steps + 1, hidden_dim);
	  S(0, 0) = static_cast <float> (((float) rand()) / (static_cast <float> (RAND_MAX / 2)) - 1);
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


void State::copy_q() {
	assert(m_do_temp);

	q_S = copy(S);
	q_r = copy(r);
	q_z = copy(z);
	q_h = copy(h);
}


void State::set_step(int time_step, State &state) {
	assert(m_do_temp);

  S = state.S.row(time_step);
  r = state.r.row(time_step);
  z = state.z.row(time_step);
  h = state.h.row(time_step);

  copy_q();
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

  for(int i = 0; i < time_steps; i++) {
    temp            = (X.row(i) * (m.U_z)) + (state.S.row(i) * (m.W_z));
    temp.eval();
    state.z.row(i)  = temp.unaryExpr(&sigmoid);
    state.z.eval();

    temp            = (X.row(i) * (m.U_r)) + (state.S.row(i) * (m.W_r));
    temp.eval();
    state.r.row(i)  = temp.unaryExpr(&sigmoid);
    state.r.eval();

    temp            = (X.row(i) * (m.U_h)) + (state.S.row(i).cwiseProduct(state.r.row(i))) * (m.W_h);
    temp.eval();
    state.h.row(i)  = temp.unaryExpr(&tanh_activation);
    state.h.eval();

    temp_hidden     = (MatrixXf::Ones(1, hidden_dim) - state.z.row(i)).cwiseProduct(state.h.row(i)) + state.z.row(i).cwiseProduct(state.S.row(i));
    temp_hidden.eval();
    state.S.row(i + 1) = temp_hidden;
    state.S.eval();

    temp_output   = state.S.row(i + 1) * (m.V);
    temp_output.eval();

    s_max          = temp_output.maxCoeff();
    temp_output    = temp_output.unaryExpr(&softmax);
    temp_output.eval();

    state.O.row(i) = temp_output / temp_output.sum();
    state.O.eval();

    temp_output = state.O.row(i);
    temp_output.eval();

		if (!do_test) {
	    state.E(0, i) += -1 * (Y.row(i).cwiseProduct(temp_output.unaryExpr(&log_matrix)).sum());
	    state.E.eval();
	  }
  }

	timers.stop("forward_propagation");
}
