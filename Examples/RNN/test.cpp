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
void test_outer_product(BaseKernel &op) {
	const int blocks = 2;
	const int vec_size = blocks*16;

	Float::Array left_outer(vec_size); 
	init_input(left_outer, primes, vec_size);

	Float::Array right_outer(vec_size); 
	Float::Array result_outer(vec_size*vec_size); 

	for (int i = 0; i < vec_size; ++i) {
	  right_outer[i] = (float) (i + 1);
	}

	op.load(&left_outer, &right_outer, &result_outer, blocks);
	op.run();

	std::string buf;
	buf << "outer product:\n";
	for (int i = 0; i < vec_size; ++i) {
	  buf << i << ": " << vector_dump(result_outer, vec_size, vec_size*i) << "\n";
	}
	warn << buf;
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
