#ifndef _INCLUDE_RNN_MODEL
#define _INCLUDE_RNN_MODEL
#include "matrix.h"
#include "scalar.h"

using namespace V3DLib;

const int M = 16; // Num of rows in matrix
const int N = 2;  // Width of matrix and length of vector, in blocks of 16

const int NumOutputNodes = 3;

/**
 * The layer sizes of the example models are:
 *
 * input : hidden : output = 30 : 5 : 3
 *
 * The kernel(s) however work with array sizes of multiples
 * of 16. We should use the smallest 16-item increments that
 * encompass the original size.
 */
struct model {
	model(int n_size, int m_size);

	float alpha = 1.0f;  // ruby version works fine with value 1

	matrix w1{M, 16*N};
	vector z1{16};
  vector bias1{16};
  vector a1{16};

	matrix w2{16, 16};
  vector bias2{16};
  vector z2{16};
  vector a2{16};

	Float::Array s_tmp{16}; // For scalar output

	vector forward(vector const &input);
	void back_prop(vector const &input, vector const &desired);
};

#endif // _INCLUDE_RNN_MODEL
