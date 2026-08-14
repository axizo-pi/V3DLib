#include "forward.h"
#include "global.h"


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
  MMatrix const &X,
  MMatrix const &Y,  // Not used in test
  State &state,
  int time_steps,
  int input_dim,
  int hidden_dim,
  int output_dim,
  bool do_test
) {
  timers.start("forward_propagation");

  MMatrix temp(1, hidden_dim);
  MMatrix temp_hidden;
  MMatrix X_row;
  MMatrix Y_row;
  MMatrix Z_row;

  for(int i = 0; i < time_steps; i++) {
    //warn << "Forward i: " << i;

    // Timing init inconsequential
    auto S_row = state.S().row(i); // Only S_row[0] set
    X_row.set(X.row(i));
    Y_row.set(Y.row(i));

    Z_row = state.z.row(i);
    assert(Z_row.is_zero());  // Apparently always zero

    temp = X_row.forward_1(m, S_row);
    state.z.row(i, temp.sigmoid());  // Profile timing minimal

    temp = X_row.forward_2(m, S_row);
    state.r.row(i, temp.sigmoid());

    MMatrix tempb = X_row.forward_3(m, S_row, state.r.row(i));
		//warn << "tempb: " << tempb.dump();
    state.h.row(i, tempb.tanh());

    // forward_4/5 timing small
		//warn << "Z_row: " << Z_row.dump();
		//warn << "S_row: " << S_row.dump();
		//warn << "state.h: " << state.h.dump();
		//warn << "state.h.row(i): " << state.h.row(i).dump();

    temp_hidden = Z_row.forward_4(S_row, state.h.row(i));
    state.S().row(i + 1, temp_hidden);   // Assumption: value at i == 0 should be retained

		//warn << "temp_hidden: " << temp_hidden.dump();
		//warn << "m.V: " << m.V.dump();  // Non-zero
    auto temp_output = temp_hidden * m.V;
		//warn << "temp_output: " << temp_output.dump();

    auto temp2 = temp_output.forward_5();
		//warn << "temp2: " << temp2.dump();

    state.O.row(i, temp2);

    if (!do_test) {
      MMatrix ret = state.E.calc_E(Y_row, state.O.row(i));
      state.E.col_E(i, ret);
    }
  }

  timers.stop("forward_propagation");
}
