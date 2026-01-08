#include "scalar.h"
#include <cmath>  // exp()
#include "tools.h"

namespace scalar {

namespace {
	
/**
 * activation function
 */
float sigmoid_int(float x) {
	return (float) (1.0/(1.0 + std::exp(-x)));
}

} // anon namespace

void frand_array(Float::Array &rhs) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    rhs[i] = frand();
  }
}


void sigmoid(Float::Array &vec, Float::Array const &bias, Float::Array &output) {
  for (int h = 0; h < (int) vec.size(); ++h) {
		output[h] = sigmoid_int(vec[h] + bias[h]); 
  }
}


/**
 * matrix width is assumed to be the same as the vector length
 */
void mult(Float::Array const &vec, Float::Array const &mat, Float::Array &res) {
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


/**
 * Using MSE to calculate loss.
 */
float loss(Float::Array const &result, Float::Array const &y) {
	assert(result.size() > 0);
	assert(result.size() == y.size());

	int width = result.size();
	float sum = 0;

	for (int i = 0; i < width; ++i) {
		sum += (result[i] - y[i])*(result[i] - y[i]);
	}

	return sum/((float) width);
}

} // namespace scalar
