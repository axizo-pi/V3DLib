#include "forward.h"
#include "global.h"
#include "Support/Helpers.h"  // bit_diff()


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
	MatrixXf ones = MatrixXf::Ones(1, hidden_dim);

  timers.start("forward_propagation");

  MMatrix temp(1, hidden_dim);
  MatrixXf temp_hidden = MatrixXf::Zero(1, hidden_dim);

  for(int i = 0; i < time_steps; i++) {
    warn << "Forward i: " << i;

    auto S_row = state.S.row(i);
    MMatrix X_row;
    X_row.set(X.row(i));
    MMatrix Y_row;
    Y_row.set(Y.row(i));
    auto z_row = state.z.row(i);

		temp = X_row.forward_1(m, S_row);

    state.z.row(i, temp.sigmoid());

		temp = X_row.forward_2(m, S_row);

    state.r.row(i, temp.sigmoid());

		MMatrix tempb = X_row.forward_3(m, S_row, state.r.row(i));
    state.h.row(i, tempb.tanh());

		MMatrix temp_hidden = z_row.forward_4(S_row, state.h.row(i));

    // Assumption: value at i == 0 should be retained
    state.S.row(i + 1, temp_hidden);

    /// Should be able to use temp_hidden directly, instead of row(i+1)
    assert(state.S.row(i + 1).same(temp_hidden));

		auto temp_output = state.S.row(i + 1) * m.V;
    temp_output.softmax();

    float temp_sum = temp_output.Xf().sum();

    state.O.row(i, temp_output/temp_sum);
    state.O.eval();

    if (!do_test) {
      MMatrix ret = state.E.calc_E(Y_row, state.O.row(i));
      state.E.col_E(i, ret);
    }
  }

  timers.stop("forward_propagation");
}
