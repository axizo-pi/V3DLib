#include "test.h"
#include "model.h"
#include "Source/Functions.h"  // rotate_sum

using namespace V3DLib;

namespace {
// Values for testing purposes
const int primes_size = 2*16;

float primes[primes_size] = {
  2,	3,	5,	7,	11,	13,	17,	19,	23,	29,	31,	37,	41,	43,	47,	53,	59,	61,	67,	71,
 73, 79,	83,	89,	97,	101,	103,	107,	109,	113,	127,	131 //	137	139	149	151	157	163	167	173
};

void init_input(Float::Array &input, float *a,  int n) {
  for (int h = 0; h < n; ++h) {
		input[h] = a[h];
	}
}

} // anon namespace


/**
 * Separate test for outer product
 */
void test_outer_product(BaseKernel &op, bool do_kernel) {
	const int blocks = 2;
	const int vec_size = blocks*16;

	Float::Array left_outer(vec_size); 
	for (int i = 0; i < vec_size; ++i) {
	  left_outer[i] = (float) (i + 1);
	}

	Float::Array right_outer(vec_size); 
	init_input(right_outer, primes, vec_size);

	Float::Array result_outer(vec_size*vec_size); 

	if (do_kernel) {
		// Call kernel directly
		op.load(&left_outer, &right_outer, &result_outer, blocks);
		op.run();

		std::string buf;
		buf << "outer product:\n";
		for (int i = 0; i < vec_size; ++i) {
		  buf << i << ": " << vector_dump(result_outer, vec_size, vec_size*i, true) << "\n";
		}
		warn << buf;
	} else {
		// Perform outer product with vector operations
		vector a1(vec_size);
		for (int i = 0; i < vec_size; ++i) {
		  a1[i] = (float) (i + 1);
		}
 		warn << "a1: " << a1.dump(true);

		vector d2(vec_size);
		for (int i = 0; i < vec_size; ++i) {
	  	d2[i] = primes[i];
		}
 		warn << "d2: " << d2.dump(true);

		auto w2_adj = a1.outer(d2);             // gradient, outer product
 		warn << "w2_adj:\n" << w2_adj.dump(true);
	}
}


void test_back_propagation(V3DLib::BaseKernel &op) {
	warn << "test_back_propagation";
	//test_outer_product(op, false);

	//
	// Output layer to hidden layer
	//
	// TODO

	//	
	// Hidden layer adjusting w1
	//
	matrix w2(16, 32);   // !!! Params reversed wrt ruby Matrix
	w2.set(0.5);
 	warn << "w2: " << w2.dump();

	vector d2(16);
	d2.set(0.25);
 	warn << "d2(" << d2.size() << "): " << d2.dump();

	// ####
	auto tmp1 = w2 * d2;
 	warn << "tmp1: " << tmp1.dump();

	vector a1(32);
	for (int i = 0; i < a1.size(); ++i) {
	  a1[i] = (float) (i + 1);
	}
 	warn << "a1: " << a1.dump(true);


	// ####
	auto d1 = a1.sigmoid_derivative(tmp1);
 	warn << "d1: " << d1.dump(true);

	vector input(16);
	input.set(1);
 	warn << "input: " << input.dump(true);

	// ####
	auto w1_adj = input.outer(d1);        // gradient, outer product; result transposed
 	warn << "w1_adj:\n" << w1_adj.dump(true);
	auto tmp2 = w1_adj.transpose();
	w1_adj = tmp2;
 	warn << "w1_adj transposed:\n" << w1_adj.dump(true);

	matrix w1(32, 16);
	w1.set(1);
 	warn << "w1:\n" << w1.dump();

	float alpha = 1;
	auto tmp = alpha*w1_adj;
 	warn << "tmp:\n" << tmp.dump(true);

	//w1 -= alpha*w1_adj;
 	//warn << "w1:\n" << w1.dump();

	warn <<  "### Till Here ###";
}


void test_vector() {
	const int blocks = 2;
	const int vec_size = blocks*16;
/*
	vector a_vec(vec_size); a_vec.set(4);
	vector b_vec(vec_size); b_vec.set(3);
	auto c_vec = a_vec - b_vec;
 	warn << "a_vec: " << a_vec.dump();
 	warn << "sub: "   << c_vec.dump();
*/	

	vector d_vec{vec_size}; d_vec.set(primes, primes_size);
	warn << "d_vec: " << d_vec.dump();

	vector e_vec{vec_size};

	for (int i = 0; i < vec_size; ++i) {
	  e_vec[i] = (float) (i + 1);
	}
	warn << "e_vec: " << e_vec.dump();

	auto result = d_vec.outer(e_vec);
	warn << "result: " << result.dump();
}
