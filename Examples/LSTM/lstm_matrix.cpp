#include "lstm_matrix.h"

/**
 * /file
 *
 * Matrix and vector definitions derived from the reference program.
 *
 * The goal is to get the class interfaces the same as the corresponding QPU classes.
 */

namespace lstm {

vector vector::operator+(const vector & b) const {
  const vector & a= *this;

  vector result(a.size());
  for (std::size_t i = 0; i < a.size(); i++) {
    result[i] = a[i] + b[i];
  }
  return result;
}


vector vector::operator*(const vector & b) const {
  const vector & a= *this;

  vector result(a.size());
  for (std::size_t i = 0; i < a.size(); i++) {
    result[i] = a[i] * b[i];
  }
  return result;
}


vector operator*(float scalar, const vector & v) {
  vector result(v.size());
  for (std::size_t i = 0; i < v.size(); i++) {
    result[i] = scalar * v[i];
  }
  return result;
}


lstm::vector matrix::operator*(const lstm::vector &v) {
  const matrix &m = *this;

  lstm::vector result(m.size(), 0.0);
  for (std::size_t i = 0; i < m.size(); i++) {
    for (std::size_t j = 0; j < v.size(); j++) {
      result[i] += m[i][j] * v[j];
    }
  }
  return result;
}


} // namespace lstm
