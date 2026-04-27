#include "lstm_matrix.h"
#include "scalar.h"
#include "Support/basics.h"
#include <cassert>
#include <cmath>    // tanh()

using std::size_t;

/**
 * /file
 *
 * Matrix and vector definitions derived from the reference program.
 *
 * The goal is to get the class interfaces the same as the corresponding QPU classes.
 */

namespace lstm {
namespace {

// Gradient clipping as mentioned in the blog
float clip_intern(float value, float min_value, float max_value) {
  return std::max(min_value, std::min(value, max_value));
}

} // anon namespace

vector vector::operator+(vector const &b) const {
  vector const &a= *this;

  vector result(a.size());
  for (std::size_t i = 0; i < a.size(); i++) {
    result[i] = a[i] + b[i];
  }
  return result;
}


/**
 * @brief By-element product of the two vectors.
 *
 * Note that this is _not_ a dot product (as I expected)
 */
vector vector::operator*(vector const &b) const {
  vector const &a= *this;

  vector result(a.size());
  for (std::size_t i = 0; i < a.size(); i++) {
    result[i] = a[i] * b[i];
  }
  return result;
}


/**
 * @brief calculate `sigmoid(*this + bias)`.
 */
vector vector::sigmoid(vector const &bias) {
  vector const &v = *this;
	assert(v.size() == bias.size());

  vector result(v.size());
  for (size_t i = 0; i < v.size(); i++) {
    result[i] = scalar::sigmoid(v[i] + bias[i]);
  }
  return result;
}


void vector::clip(float extreme) {
	assert(extreme > 0);
	float min_value = -extreme;
	float max_value = extreme;

  for (size_t i = 0; i < size(); i++) {
    at(i) = clip_intern(at(i), min_value, max_value);
  }
}


std::string vector::dump_dim() const {
	std::string ret;

	ret << "vec(" << size() << ")";

	return ret;
}


std::string vector::dump() const {
	std::string ret;

	ret << dump_dim() << "[";

	for (int i = 0; i < (int) size(); ++i) {
		ret << at(i) << ", ";
	}

	ret << "]";

	return ret;
}


vector operator*(float scalar, vector const &v) {
  vector result(v.size());

  for (std::size_t i = 0; i < v.size(); i++) {
    result[i] = scalar * v[i];
  }
  return result;
}


lstm::vector matrix::operator*(lstm::vector const &v) {
  matrix const &m = *this;

  lstm::vector result(m.size(), 0.0);
  for (std::size_t i = 0; i < m.size(); i++) {
    for (std::size_t j = 0; j < v.size(); j++) {
      result[i] += m[i][j] * v[j];
    }
  }
  return result;
}


std::string matrix::dump_dim() const {
	assert(size() >= 1);
	auto width = at(0).size();

	std::string ret;
	ret << "(" << size() << ", " << width << ")";
	return ret;
}


std::string matrix::dump() const {
	assert(size() >= 1);
	auto width = at(0).size();

	std::string ret;
	ret << dump_dim() << " [\n  ";

  for (std::size_t i = 0; i < size(); i++) {
		ret << i << ": [";
    for (std::size_t j = 0; j < width; j++) {
      ret << at(i)[j] << ", ";
    }

		ret << "]\n  ";
  }

	ret << "]";

	return ret;
}


//
// Helper functions
//

vector tanh(vector const &v) {
  vector result(v.size());
  for (size_t i = 0; i < v.size(); i++) {
    result[i] = (float) std::tanh(v[i]);
  }
  return result;
}

/**
 * Input is `v = sigmoid(x)`
 */
vector dsigmoid(vector const &v) {
  vector result(v.size());
  for (size_t i = 0; i < v.size(); i++) {
    result[i] = v[i] * (1.0f - v[i]);
  }
  return result;
}


/**
 * Input is `y = tanh(x)`
 */
vector dtanh(vector const &v) {
  vector result(v.size());
  for (size_t i = 0; i < v.size(); i++) {
    result[i] = 1.0f - v[i] * v[i];
  }
  return result;
}


//
// Vector operations
//

// Vector concatenation
vector concat(vector const &a, vector const &b) {
  vector result = a;
  result.insert(result.end(), b.begin(), b.end());
  return result;
}

} // namespace lstm
