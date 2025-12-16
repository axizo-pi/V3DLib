/**
 * Support for Reveirse Neural Networks.
 *
 * Adapted from: https://www.geeksforgeeks.org/numpy/implementation-of-neural-network-from-scratch-using-numpy/
 */
#include "model.h"
#include "Support/basics.h"
#include "Source/Functions.h"  // rotate
#include "Support/Timer.h"

float a[3][30] = {
{	
	0, 0, 1, 1, 0, 0,
 	0, 1, 0, 0, 1, 0,
 	1, 1, 1, 1, 1, 1,
 	1, 0, 0, 0, 0, 1,
 	1, 0, 0, 0, 0, 1,
}, {
	0, 1, 1, 1, 1, 0,
  0, 1, 0, 0, 1, 0,
  0, 1, 1, 1, 1, 0,
  0, 1, 0, 0, 1, 0,
  0, 1, 1, 1, 1, 0
}, {				
	0, 1, 1, 1, 1, 0,
  0, 1, 0, 0, 0, 0,
  0, 1, 0, 0, 0, 0,
  0, 1, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 0
}};

float labels[3][3] = {
	{1, 0, 0},
  {0, 1, 0},
  {0, 0, 1}
};

void init_input(Float::Array &input, float *a,  int n) {
  for (int h = 0; h < n; ++h) {
		input[h] = a[h];
	}
}

namespace {
	// Values for testing purposes

	const int primes_size = 2*16;

	float primes[primes_size]=
	{ 2,	3,	5,	7,	11,	13,	17,	19,	23,	29,	31,	37,	41,	43,	47,	53,	59,	61,	67,	71,
	 73,	79,	83,	89,	97,	101,	103,	107,	109,	113,	127,	131 //	137	139	149	151	157	163	167	173
	};

} // anon namespace


/**
 * Separate test for outer product
 */
void test_outer_product(BaseKernel &op) {
	Float::Array left_outer(2*16); 
	init_input(left_outer, primes, 2*16);

	Float::Array right_outer(2*16); 
	Float::Array result_outer((2*16)*(2*16)); 

	for (int i = 0; i < 2*16; ++i) {
	  right_outer[i] = (float) (i + 1);
	}

	op.load(&left_outer, &right_outer, &result_outer);
	op.run();

	std::string buf;
	buf << "outer product:\n";
	for (int i = 0; i < 2*16; ++i) {
	  buf << i << ": " << vector_dump(result_outer, 2*16, 2*16*i) << "\n";
	}
	warn << buf;
}


void test_vector() {
/*	
	vector<2> a_vec; a_vec.set(3);
	vector<2> b_vec; b_vec.set(-7);
	auto c_vec = a_vec - b_vec;
 	warn << "a_vec: " << a_vec.dump();
 	warn << "sub: " << c_vec.dump();
*/	
	vector<2> d_vec; d_vec.set(primes, primes_size);
	//warn << "d_vec: " << d_vec.dump();

	vector<2> e_vec;

	for (int i = 0; i < 2*16; ++i) {
	  e_vec[i] = (float) (i + 1);
	}

	warn << "e_vec: " << e_vec.dump();

	auto result = d_vec.outer(e_vec);

	warn << "result: " << result.dump();
}


int main(int argc, const char *argv[]) {
  settings().init(argc, argv);

	model<2> k_model(M);

	//test_outer_product(vector<2>::op_kernel());
	//test_vector();

	vector<N> input;
	input.set(a[0], 30);


	auto result = k_model.forward(input);

 // warn << "Scalar a2 : " << vector_dump(k_model.s_tmp, 16);
 // warn << "kernel res: " << result.dump();

	k_model.back_prop(input, result);

  return 0;
}
