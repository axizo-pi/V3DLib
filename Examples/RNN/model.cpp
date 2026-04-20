#include "model.h"

model::model(int n_size, int m_size) : s_tmp(n_size) {		
	w1.frand();
	w2.frand();
	bias1.frand();
	bias2.frand();
}


/**
 * Do a forward step given the input.
 *
 * ------
 *
 * NOTES
 * -----
 *
 * mat.transpose() * vec;  - is equivalent to 'vec * mat' in model.rb
 *
 * It does however add overhead due to the transposition. **MAKE IT WORK** is where we're at.
 */
vector model::forward(vector const &input) {
	V3DLib::timers.start("forward transpose");
  z1 = w1.transpose() * input;    // Input from layer 1 
	V3DLib::timers.stop("forward transpose");

	V3DLib::timers.start("forward sigmoid");
	a1 = z1.sigmoid(bias1);         // Output of layer 2
	V3DLib::timers.stop("forward sigmoid");

	V3DLib::timers.start("forward transpose");
  z2 = w2.transpose() * a1;
	V3DLib::timers.stop("forward transpose");

	V3DLib::timers.start("forward sigmoid");
	a2 = z2.sigmoid(bias2);         // Output of out layer
	V3DLib::timers.stop("forward sigmoid");

	return a2;
}


void model::back_prop(vector const &input, vector const &desired) {
	V3DLib::timers.start("back_prop forward");
	auto la2 = forward(input);  // Same as member a2; expected
	V3DLib::timers.stop("back_prop forward");

	//
	// Output layer to hidden layer
	//
	V3DLib::timers.start("back_prop Output Layer to hidden layer");
	auto d2     = la2 - desired;             // error in output layer
	auto w2_adj = a1.outer(d2);              // gradient, outer product
	auto w2_tmp = w2 - alpha*w2_adj;
	bias2      -= alpha * d2;
	V3DLib::timers.stop("back_prop Output Layer to hidden layer");

	//	
	// Hidden layer adjusting w1
	//
	V3DLib::timers.start("back_prop Hidden layer adjusting w1");
	auto tmp1   = w2 * d2;
	auto d1     = a1.sigmoid_derivative(tmp1);
	auto w1_adj = input.outer(d1);           // gradient, outer product;
	w1         -= alpha*w1_adj;
	bias1      -= alpha * d1;
	w2          = w2_tmp;
	V3DLib::timers.stop("back_prop Hidden layer adjusting w1");

// 	warn << "w2: " << w2.dump();
}
