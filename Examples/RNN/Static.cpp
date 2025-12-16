#include "Static.h"
#include <cmath>  // exp()
#include "tools.h"


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


void scalar_sigmoid(Float::Array &vec, Float::Array const &bias, Float::Array &output) {
  for (int h = 0; h < (int) vec.size(); ++h) {
		output[h] = sigmoid(vec[h] + bias[h]); 
  }
}



/**
 * matrix width is assumed to be the same as the vector length
 */
void run_scalar(Float::Array const &vec, Float::Array const &mat, Float::Array &res) {
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
