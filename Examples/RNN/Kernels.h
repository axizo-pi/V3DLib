#ifndef _INCLUDE_RNN_KERNELS
#define _INCLUDE_RNN_KERNELS
#include "Source/Float.h"

using namespace V3DLib;

template<int const N>
void kernel_sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out) {

  for (int h = 0; h < N; ++h) {
		Float x = *in;

		x += *bias;
		x = (1.0/(1.0 + exp_e(-1.0*x)));

		*out = x;

		in.inc();
		bias.inc();
		out.inc();
	}
}


template<int const M, int const N>
void kernel(Float::Ptr input, Float::Ptr mat, Float::Ptr result) {
  assert(N > 0);
  assert(M > 0);

  Float tmp[M];
  Float::Ptr row[M];
  Int offset2 = 4*16*N;

  for (int h = 0; h < M; ++h) {
    tmp[h] = 0;
    row[h] = mat.offset(h*offset2);
  }

  for (int i = 0; i < N; ++i) {
    Float v = *input;
  
    for (int h = 0; h < M; ++h) {
      tmp[h] += (*row[h])*v;
    }

    if (i < (N - 1)) {
      for (int h = 0; h < M; ++h) {
        row[h].inc();
      }

      input.inc();
    }
  }

  Float res = 0;

  for (int h = 0; h < M; ++h) {
    rotate_sum(tmp[h], tmp[h]);
    res.set_at(h, tmp[h]);
  }

  *result = res;
}


template<int const N> // length of input vectors, in blocks of 16
void outer_product(Float::Ptr left, Float::Ptr right, Float::Ptr out_matrix) {
	left -= index();
	//Float right_out = toFloat(2*(1 + index()));

  for (int i = 0; i < 16*N; ++i) {
		Float::Ptr start = right;

  	for (int j = 0; j < N; ++j) {
			*out_matrix = *left * *start;
			out_matrix.inc(); start.inc();
		}

		++left;
	}
}


template<int const N> // length of input vectors, in blocks of 16
void vector_sub(Float::Ptr left, Float::Ptr right, Float::Ptr out) {
	Float tmp;

  for (int i = 0; i < N; ++i) {
		tmp = *left - *right; 
		*out = tmp;

		left.inc();
		right.inc();
		out.inc();
	}
}

#endif //  _INCLUDE_RNN_KERNELS
