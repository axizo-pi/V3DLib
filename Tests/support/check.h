#ifndef _TEST_SUPPORT_CHECK_H
#define _TEST_SUPPORT_CHECK_H

template<typename Array>
std::string showResult(Array &result, int index, int size = 16) {
  REQUIRE(size % 16 == 0);
  std::ostringstream buf;

  buf << "result  : ";
  for (int j = 0; j < size; j++) {
    buf << result[size*index + j] << ", ";
  }

  return buf.str();
}


template<typename T>
std::string showExpected(const std::vector<T> &expected) {
  std::ostringstream buf;

  buf << "expected: ";
  for (int j = 0; j < (int) expected.size(); j++) {
    buf << expected[j] << ", ";
  }

  return buf.str();
}


/**
 * Compare `result` starting at `index` with expected value.
 */
template<typename T1, typename T2>
void check_vector(
  V3DLib::SharedArray<T2> &result,
  int index,
  std::vector<T1> const &expected,
  float precision = 0.0f
) {
  REQUIRE(expected.size() % 16 == 0);  // size must match 16-vectors

  bool passed = true;
  int first_j = 0;
  float max_diff = 0;

  for (int j = 0; j < (int) expected.size(); ++j) {
    float diff = abs((float) result[16*index + j] - (float) expected[j]);

    if (max_diff < diff) {
      max_diff = diff;
    }

    if (passed) {
      if (diff > precision) {
        first_j = j;
        passed = false;
      }
    }
  }

  INFO("index: " << index << ", first j: " << first_j);
  INFO("max diff: " << max_diff);
  INFO(showResult(result, index, (int) expected.size()));
  INFO(showExpected(expected));
  REQUIRE(passed);
}


/**
 * Overload which assumes that all elements of the 16-value block have the same values
 */
template<typename T>
void check_vector(V3DLib::SharedArray<T> &result, int index, int expected, float precision = 0.0f) {
  std::vector<T> vec;
  vec.resize(16);

  for (int i = 0; i < (int) vec.size(); ++i) {
    vec[i] = expected;
  }

  check_vector(result, index, vec, precision);
}


template<typename T>
void check_vectors(
  V3DLib::SharedArray<T> &result,
  std::vector<std::vector<T>> const &expected,
  float precision = 0.0f
) {
  for (int index = 0; index < (int) expected.size(); ++index) {
    check_vector(result, index, expected[index], precision);
  }
}


#endif // _TEST_SUPPORT_CHECK_H
