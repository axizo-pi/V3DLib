#include "model.h"

model::model(int n_size, int m_size) : s_tmp(n_size) {
	w1.frand();
	w2.frand();
	bias1.frand();
	bias2.frand();
}


vector model::forward(vector const &input) {
 	//warn << "frrand_count:" << frrand_count();
 	warn << "w1: " << w1.dump();
  z1 = w1 * input;
 	warn << "z1: " << z1.dump();
	warn << "Till Here" << thrw;

	//scalar::mult(input.arr(), w1.arr(), s_tmp);
  //warn << "scalar z1: " << vector_dump(s_tmp, 16);

  //warn << "bias1: " << bias1.dump();
	a1 = z1.sigmoid(bias1);
	//scalar::sigmoid(z1.arr(), bias1.arr(), s_tmp);
  //warn << "kernel sigmoid: " << a1.dump();
  //warn << "scalar sigmoid: " << vector_dump(s_tmp, 16);

  z2 = w2 * a1;
	//scalar::mult(a1.arr(), w2.arr(), s_tmp);
  //warn << "kernel z2: " << z2.dump();
  //warn << "scalar z2: " << vector_dump(s_tmp, 16);

	a2 = z2.sigmoid(bias2);
	//scalar::sigmoid(z2.arr(), bias2.arr(), s_tmp);
  //warn << "kernel sigmoid: " << a2.dump();
  //warn << "scalar sigmoid: " << vector_dump(s_tmp, 16);

	return a2;
}


void model::back_prop(vector const &input, vector const &desired) {
 	//warn << "Pre  a2: " << a2.dump();
	auto la2 = forward(input);  // Same as member a2; expected
 	//warn << "Post la2: " << la2.dump();

	//
	// Output layer to hidden layer
	//
	auto d2 = la2 - desired;                // error in output layer

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
