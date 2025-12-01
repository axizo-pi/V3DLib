/**
 * Support for Revese Neural Networks
 */
#include "Support/Settings.h"
#include "Source/Float.h"
#include "Support/basics.h"
#include "Kernel.h"
#include "Source/Functions.h"  // rotate
#include "Support/Timer.h"
#include <cmath>

using namespace V3DLib;
using namespace Log;

const int N = 16;  // Width of matrix and length of vector
const int M = 16;  // Num of rows in matrix

Settings settings;

/**
 * Source: https://stackoverflow.com/questions/686353/random-float-number-generation
 */
float frand() {
  static bool did_init = false;
  if (!did_init) {
    srand (static_cast <unsigned> (time(0)));
    did_init = true;
  }

  // Generate a number from 0.0 to 1.0, inclusive.
  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

/*
  // Generate a number from 0.0 to some arbitrary float, X
  const float X = 1.0f;
  float r2 = static_cast <float> (rand()) / (static_cast <float> (((float) RAND_MAX)/X));

  // Generate a number from some arbitrary LO to some arbitrary HI:
  const float LO = 0.0f;
  const float HI = 1.0f;
  float r3 = LO + static_cast <float> (rand()) /( static_cast <float> (((float) RAND_MAX)/(HI-LO)));
*/

  return r;
}


void frand_array(Float::Array &rhs) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    rhs[i] = frand();
  }
}


/**
 * activation function
 */
float sigmoid(float x) {
	return (float) (1.0/(1.0 + std::exp(-x)));
}


void scalar_sigmoid(float *vec, int size, Float::Array const &bias) {
  for (int h = 0; h < size; ++h) {
		vec[h] = sigmoid(vec[h] - bias[h]); 
  }
}



/**
 * matrix width is assumed to be the same as the vector length
 */
void run_scalar(Float::Array const &mat, Float::Array const &vec, float *res) {
  int height = mat.size()/vec.size();
  assert(height*vec.size() == mat.size());

  for (int h = 0; h < height; ++h) {
    res[h] = 0;

    int offset = h*vec.size();
    for (int w = 0; w < (int) vec.size(); ++w) {
      res[h] += mat[offset + w] * vec[w];
    }
  }
}


void kernel_sigmoid(Float::Ptr in, Float::Ptr out) {
	Float x = *in;

	x = (1.0/(1.0 + exp(-1.0*x)));

	*out = x;
}


template<int const M, int const N>
void kernel(Float::Ptr in_mat, Float::Ptr vec, Float::Ptr result) {
  assert(N > 0);
  assert(M > 0);

  Float tmp[M];
  Float::Ptr row[M];
  Int offset2 = 4*16*N;

  for (int h = 0; h < M; ++h) {
    tmp[h] = 0;
    row[h] = in_mat.offset(h*offset2);
  }

  for (int i = 0; i < N; ++i) {
    Float v = *vec;
  
    for (int h = 0; h < M; ++h) {
      tmp[h] += (*row[h])*v;
    }

    if (i < (N - 1)) {
      for (int h = 0; h < M; ++h) {
        row[h].inc();
      }

      vec.inc();
    }
  }

  Float res = 0;

  for (int h = 0; h < M; ++h) {
    rotate_sum(tmp[h], tmp[h]);
    res.set_at(h, tmp[h]);
  }

  *result = res;
}


int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

  Float::Array matrix(M*16*N);
  Float::Array vector(16*N);
  frand_array(matrix);
  frand_array(vector);

  Float::Array bias(16*N);
  frand_array(bias);

  float scalar_res[M] = {0};
  run_scalar(matrix, vector, scalar_res);


  Float::Array result(M);
  Float::Array sigmoid(M);

  auto k = compile(kernel<M, N>, settings);
  k.load(&matrix, &vector, &result);

	auto k_sigmoid = compile(kernel_sigmoid, settings);
	k_sigmoid.load(&result, &sigmoid);


  {
    Timer timer("Matrix mult");
    k.run();
		k_sigmoid.run();
    timer.end();

    int flops = M*(16*N + 16*(N - 1));  // num_rows*(num_mults + num_adds);
	  flops += M*5;                       // sigmoid (1.0/(1.0 + exp(-1.0*x)));

    warn << "MFLOPS: " << ((float) flops)/timer.diff()/1.0e6;
    //warn << "Timer diff: " << timer.diff();
  }

  std::string buf;
  for (int h = 0; h < (int) result.size(); ++h) {
    buf << scalar_res[h] << ", ";
  }
  warn << "scalar result: " << buf;

  buf.clear();
  for (int i = 0; i < (int) result.size(); ++i) {
    buf << result[i] << ", ";
  }
  warn << "kernel result: " << buf;

	scalar_sigmoid(scalar_res, M, bias);

  buf.clear();
  for (int h = 0; h < M; ++h) {
    buf << scalar_res[h] << ", ";
  }
  warn << "Scalar sigmoid: " << buf;

  buf.clear();
  for (int i = 0; i < (int) sigmoid.size(); ++i) {
    buf << sigmoid[i] << ", ";
  }
  warn << "kernel sigmoid: " << buf;

  return 0;
}
