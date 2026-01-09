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
	vector d_vec{32}; d_vec.set(primes, primes_size);
	//warn << "d_vec: " << d_vec.dump();

	vector e_vec{32};

	for (int i = 0; i < 2*16; ++i) {
	  e_vec[i] = (float) (i + 1);
	}

	warn << "e_vec: " << e_vec.dump();

	auto result = d_vec.outer(e_vec);

	warn << "result: " << result.dump();
}
