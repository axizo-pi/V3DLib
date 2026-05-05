#ifndef _GRU_MODEL_H
#define _GRU_MODEL_H
#include "common.h"

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
	bool do_test = false);

#endif //  _GRU_MODEL_H
