#include "convert.h"
#include "../Lib/helpers.h"  // resize_16()

/**
 * /file
 *
 * Support routines for converting lstm vector/matrix to qpu
 */

using namespace qpu;


/**
 * @brief Initialize an LSTM vector from a QPU vector
 *
 * @param rhs    QPU vector to copy from
 * @param offset Index of first element to copy.
 * @param length Number of elements to copy. If -1, copy all elements.
 */
lstm::vector copy(qpu::vector const &rhs, int offset, int length) {
  assert(offset >= 0);
  if (length == -1) {
    length = rhs.size() - offset;
  }
  assert(length + offset <= rhs.size());

  lstm::vector ret(length, 0.0f);

  for (int i = 0; i < length; ++i) {
    ret[i] = rhs[i + offset];
  }

  return ret;
}


/**
 * @brief Initialize a QPU vector from an LSTM vector
 *
 * The underlying type for `lstm::vector` is `std::vector`.
 */
qpu::vector copy(lstm::vector const &rhs) {
  int size = resize_16((int) rhs.size());
  qpu::vector ret(size);
  ret.set(0.0f);

  for (int i = 0; i < (int) rhs.size(); ++i) {
    ret[i] = rhs[i];
  }

  return ret;
}


/**
 * @brief Initialize a QPU matrix from an LSTM matrix
 *
 * The underlying type for `lstm::matrix` is `std::vector<std::vector<float>>`.
 */
qpu::matrix copy(lstm::matrix const &rhs) {
  int height = resize_16((int)  rhs.size());
  assert(rhs.size() >= 1);
  auto width = resize_16((int)rhs[0].size());

  qpu::matrix ret(height, (int) width);
  ret.set(0.0f);

  for (std::size_t i = 0; i < rhs.size(); i++) {
    for (int j = 0; j < (int) rhs[0].size(); j++) {
      ret.at((int) i, (int) j) = rhs[i][j];
    }
  }

  return ret;
}


qpu::vector qpu_concat(lstm::vector const &x, lstm::vector const &y) {
  int size = resize_16((int) (x.size() + y.size()));
  qpu::vector ret(size);
  ret.set(0.0f);

  for (int i = 0; i < (int) x.size(); ++i) {
    ret[i] = x[i];
  }

  int offset = (int) x.size();

  for (int i = 0; i < (int) y.size(); ++i) {
    ret[i + offset] = y[i];
  }

  return ret;
}


/**
 * Compares only elements in lstm vector;
 * qpu vector is aligned to 16-bits, so might have more elements
 */
bool same(qpu::vector const &lhs, lstm::vector const &rhs, float precision) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    if (!check_precision(lhs[i], rhs[i], precision)) {
      warn << "Fail same(qpu::vector, lstm::vector), index: " << i;
      return false;
    }      
  }

  return true;
}


bool same(lstm::vector const &lhs, lstm::vector const &rhs, float precision) {
  if (lhs.size() != rhs.size()) {
    warn << "Fail same(lstm::vector, lstm::vector), sizes differ";
    return false;
  }

  for (int i = 0; i < (int) rhs.size(); ++i) {
    if (!check_precision(lhs[i], rhs[i], precision)) {
      warn << "Fail same(lstm::vector, lstm::vector), index: " << i;
      return false;
    }      
  }

  return true;
}


/**
 * This checks takes almost half the total runtime.
 * However, I don't see what else could be optimized here.
 */
bool same(qpu::matrix const &lhs, lstm::matrix const &rhs, float precision) {
  //V3DLib::timers.start("same(matrix,matrix)");

  int height = (int) rhs.size();
  assert(height >= 1);
  int width = (int) rhs[0].size();

  for (int i = 0; i < height; i++) {
    auto const &row = rhs[i];

    for (int j = 0; j < width; j++) {
      if (!check_precision(lhs.at(i, j), row[j], precision)) {
        warn << "Fail same(qpu::matrix, qpu::matrix), (i,j): (" << i << ", " << j << ")";
        return false;
      }
    }
  }

  //V3DLib::timers.stop("same(matrix,matrix)");
  return true;
}


