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
  z1 = w1.transpose() * input;    // Input from layer 1 
	a1 = z1.sigmoid(bias1);         // Output of layer 2
  z2 = w2.transpose() * a1;
	a2 = z2.sigmoid(bias2);         // Output of out layer

	return a2;
}


void model::back_prop(vector const &input, vector const &desired) {
	auto la2 = forward(input);  // Same as member a2; expected

	//
	// Output layer to hidden layer
	//
	auto d2 = la2 - desired;                // error in output layer

 	warn << "d2: " << d2.dump();
	warn << "Till Here" << thrw;

 	// Only top 3 elements of error (d2) are significant, zap the rest
	for (int i = NumOutputNodes; i < d2.size(); ++i) {
		d2[i] = 0;
	}
 	//warn << "d2: " << d2.dump();

	auto w2_adj = a1.outer(d2);             // gradient, outer product
 	//warn << "w2_adj:\n" << w2_adj.dump();

	auto w2_tmp = w2 - alpha*w2_adj;
 	//warn << "w2_tmp:\n" << w2_tmp.dump();
	
	bias2 -= alpha * d2;
 	//warn << "bias2: " << bias2.dump();

	//	
	// Hidden layer adjusting w1
	//
	auto tmp1 = w2 * d2;
 	//warn << "tmp1: " << tmp1.dump();

	auto d1 = a1.sigmoid_derivative(tmp1);
	warn << "d1: " << d1.dump();

	auto w1_adj = input.outer(d1);        // gradient, outer product; result transposed
 	warn << "w1_adj:\n" << w1_adj.dump();
	auto tmp2 = w1_adj.transpose();
	w1_adj = tmp2;
 	warn << "w1_adj transposed:\n" << w1_adj.dump(true);

	//w1 -= alpha*w1_adj;
	auto tmp = alpha*w1_adj;
 	warn << "tmp:\n" << tmp.dump();
 	warn << "w1:\n" << w1.dump();

	warn <<  "### Till Here ###";

	w1 -= alpha*w1_adj;

 	//warn << "w1 post:\n" << w1.dump();

 	//warn << "bias1 pre :" << bias1.dump();
	bias1 -= alpha * d1;
 	//warn << "bias1 post:" << bias1.dump();

	w2 = w2_tmp;
}
