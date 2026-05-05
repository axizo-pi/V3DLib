#include "model.h"

namespace {

float max = 0;

float softmax(float x) {
    return (float) exp(x - max);
}

} // anon namespace

void forward_propagation(
  MatrixXf& U_z,
	MatrixXf& U_r,
	MatrixXf& U_h,
	MatrixXf& W_z,
	MatrixXf& W_r,
	MatrixXf& W_h,
	MatrixXf& V,
	MatrixXf& X,
	MatrixXf& Y,  // Not used in test
	MatrixXf& O,
	MatrixXf& S,
	MatrixXf& E,  // Not used in test
	MatrixXf& z,
	MatrixXf& r,
	MatrixXf& h,
	int time_steps,
	int input_dim,
	int hidden_dim,
	int output_dim,
	bool do_test
) {
	timers.start("forward_propagation");

    /* Forward propagation Step - returns z, r, h, S, O, E */
    /*
    * z_t = ReLU[(X_t * U_z) + (S_t-1 * W_z)]
    * r_t = ReLU[(X_t * U_r) + (S_t-1 * W_r)]
    * h_t = ReLU[(X_t * U_h) + ((S_t-1 o r_t) * W_h)]
    * S_t = tanh[((1 - z_t) o h_t) + (z_t o S_t-1)]
    * O = softmax(S_t * V)
    * E = - 1/N sum(Y o log(O))
    * where * is matrix multiplication and o is componentwise multiplication
    */

    MatrixXf temp           = MatrixXf::Zero(1, hidden_dim);
    MatrixXf temp_output    = MatrixXf::Zero(1, output_dim);
    MatrixXf temp_hidden    = MatrixXf::Zero(1, hidden_dim);
    for(int i = 0; i < time_steps; i++) {

        temp            = (X.row(i) * (U_z)) + (S.row(i) * (W_z));
        temp.eval();
        z.row(i)        = temp.unaryExpr(&sigmoid);
        z.eval();

        temp            = (X.row(i) * (U_r)) + (S.row(i) * (W_r));
        temp.eval();
        r.row(i)        = temp.unaryExpr(&sigmoid);
        r.eval();

        temp            = (X.row(i) * (U_h)) + (S.row(i).cwiseProduct(r.row(i))) * (W_h);
        temp.eval();
        h.row(i)        = temp.unaryExpr(&tanh_activation);
        h.eval();

        temp_hidden     = (MatrixXf::Ones(1, hidden_dim) - z.row(i)).cwiseProduct(h.row(i)) + z.row(i).cwiseProduct(S.row(i));
        temp_hidden.eval();
        S.row(i + 1)    = temp_hidden;
        S.eval();

        temp_output     = S.row(i + 1) * (V);
        temp_output.eval();

        max             = temp_output.maxCoeff();
        temp_output     = temp_output.unaryExpr(&softmax);
        temp_output.eval();

        O.row(i)        = temp_output / temp_output.sum();
        O.eval();

        temp_output     = O.row(i);
        temp_output.eval();

				if (!do_test) {
	        E(0, i)         += -1 * (Y.row(i).cwiseProduct(temp_output.unaryExpr(&log_matrix)).sum());
	        E.eval();
				}
    }

		// if (!do_test) {
    //   E(0, 0)         = -1 * (Y.row(0).cwiseProduct(temp_output.unaryExpr(&log_matrix)).sum());
    //   E.eval();
		// }

	timers.stop("forward_propagation");
}
