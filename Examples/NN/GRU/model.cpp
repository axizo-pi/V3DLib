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
	timers.start("divide_matrix");

  for (int i = 0; i < gradient_total.rows(); ++i) {
    for (int j = 0; j < gradient_total.cols(); ++j) {
      gradient_total(i, j) = ( gradient(i, j) / ( sqrt(cache(i, j)) + 0.00000001f) );
    }
  }

	timers.stop("divide_matrix");
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
  read_binary_matrix("Weights/V"  + postfix, V);
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
	MatrixXf tmp;

  init_matrix(tmp, input_dim, hidden_dim); U_z.set(tmp);
  init_matrix(tmp, input_dim, hidden_dim); U_r.set(tmp);
  init_matrix(tmp, input_dim, hidden_dim); U_h.set(tmp);

  init_matrix(tmp, hidden_dim, hidden_dim); W_z.set(tmp);
  init_matrix(tmp, hidden_dim, hidden_dim); W_r.set(tmp);
  init_matrix(tmp, hidden_dim, hidden_dim); W_h.set(tmp);

  init_matrix(V, hidden_dim, output_dim);

	eval();
	warn << "Done Model::init()\n" << dump_dim();
}


void Model::init_zeroes(int input_dim, int hidden_dim, int output_dim, bool do_eval) {
  auto zero   = MatrixXf::Zero(input_dim, hidden_dim);
	auto zero_h = MatrixXf::Zero(hidden_dim, hidden_dim);

  U_z.set(zero);
  U_r.set(zero);
  U_h.set(zero);

  W_z.set(zero_h);
  W_r.set(zero_h);
  W_h.set(zero_h);

  V = MatrixXf::Zero(hidden_dim, output_dim);

	if (do_eval) {
		eval();
	}
}


void Model::init_ones(int input_dim, int hidden_dim, int output_dim) {
  auto ones   = MatrixXf::Ones(input_dim, hidden_dim);
  auto ones_h = MatrixXf::Ones(hidden_dim, hidden_dim);

  U_z.set(ones);
  U_r.set(ones);
  U_h.set(ones);

  W_z.set(ones_h);
  W_r.set(ones_h);
  W_h.set(ones_h);

  V = MatrixXf::Ones(hidden_dim, output_dim);

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

  V   = decay * V   + (1 - decay) * (grad.V  .cwiseProduct(grad.V  )).eval();
  eval();

	//timers.stop("cache_decay");
}


void Model::divide(Model &grad, Model &cache) {
	MatrixXf tmp = U_z.Xf();
  divide_matrix(tmp, grad.U_z.Xf(), cache.U_z.Xf());
	U_z.set(tmp);

	tmp = U_r.Xf();
  divide_matrix(tmp, grad.U_r.Xf(), cache.U_r.Xf());
	U_r.set(tmp);

	tmp = U_h.Xf();
  divide_matrix(tmp, grad.U_h.Xf(), cache.U_h.Xf());
	U_h.set(tmp);

	tmp = W_z.Xf();
  divide_matrix(tmp, grad.W_z.Xf(), cache.W_z.Xf());
	W_z.set(tmp);

	tmp = W_r.Xf();
  divide_matrix(tmp, grad.W_r.Xf(), cache.W_r.Xf());
	W_r.set(tmp);

	tmp = W_h.Xf();
  divide_matrix(tmp, grad.W_h.Xf(), cache.W_h.Xf());
	W_h.set(tmp);

  divide_matrix(V  , grad.V  , cache.V);

  eval();
}


void Model::adjust_learning_rate(float learning_rate, Model &rhs) {
	timers.start("adjust_learning_rate");

  //U_z -= learning_rate * rhs.U_z;
  U_z.set(U_z.Xf() - learning_rate * rhs.U_z.Xf());

  U_r.set(U_r.Xf() - learning_rate * rhs.U_r.Xf());
  U_h.set(U_h.Xf() - learning_rate * rhs.U_h.Xf());
  W_z.set(W_z.Xf() - learning_rate * rhs.W_z.Xf());
  W_r.set(W_r.Xf() - learning_rate * rhs.W_r.Xf());
  W_h.set(W_h.Xf() - learning_rate * rhs.W_h.Xf());

  V   -= learning_rate * rhs.V;

	timers.stop("adjust_learning_rate");
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
    auto zero = MatrixXf::Zero(1, hidden_dim);

    S.set(zero);
    z.set(zero);
    r.set(zero);
    h.set(zero);

	} else {
	  auto zero = MatrixXf::Zero(time_steps, hidden_dim);

	  E = MatrixXf::Zero(1, time_steps);
	  z.set(zero);
	  r.set(zero);
  	h.set(zero);
	  O = MatrixXf::Zero(time_steps, output_dim);

	  MatrixXf tmp_S = MatrixXf::Zero(time_steps + 1, hidden_dim);
	  tmp_S(0, 0) = static_cast <float> (((float) rand()) / (static_cast <float> (RAND_MAX / 2)) - 1);
		S.set(tmp_S);
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


void State::set_step(int time_step, State &state) {
	assert(m_do_temp);

  S.set(state.S.row(time_step));
  r.set(state.r.row(time_step));
  z.set(state.z.row(time_step));
  h.set(state.h.row(time_step));
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
		auto S_row = state.S.Xf().row(i);

    temp            = (X.row(i) * (m.U_z.Xf())) + (S_row * (m.W_z.Xf()));
    temp.eval();
    state.z.row(i, temp.unaryExpr(&sigmoid));

    temp            = (X.row(i) * (m.U_r.Xf())) + (S_row * (m.W_r.Xf()));
    temp.eval();
    state.r.row(i, temp.unaryExpr(&sigmoid));

    temp            = (X.row(i) * (m.U_h.Xf())) + (S_row.cwiseProduct(state.r.row(i))) * (m.W_h.Xf());
    temp.eval();
    state.h.row(i, temp.unaryExpr(&tanh_activation));

    temp_hidden     = (MatrixXf::Ones(1, hidden_dim) - state.z.row(i)).cwiseProduct(state.h.row(i)) + state.z.row(i).cwiseProduct(state.S.row(i));
    temp_hidden.eval();
    state.S.row(i + 1, temp_hidden);

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
