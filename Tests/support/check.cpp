#include "check.h"
#include "Support/Helpers.h"  // bit_diff()
#include <cmath>              // isnan(), isinf()

/**
 * @brief Compare `result` starting at `index` with expected value.
 *
 * Comparison is done with bit difference.
 */
void check_vector_b(
  V3DLib::SharedArray<float> &result,
  int index,
  std::vector<float> const &expected,
  int bit_precision
) {
  REQUIRE(expected.size() % 16 == 0);  // size must match 16-vectors

  bool passed = true;
  int first_j = 0;
  int  max_diff = -1;

  for (int j = 0; j < (int) expected.size(); ++j) {
    float val1 = result[16*index + j];
		float val2 = expected[j];

		// ignore special values if they match
		if ((std::isnan(val1) && std::isnan(val2))
		 || (std::isinf(val1) && std::isinf(val2))
		) {
			continue;
		}

    int diff = V3DLib::bit_diff(val1, val2, bit_precision);

    if (max_diff < diff) {
      max_diff = diff;
    }

    if (passed) {
      if (diff > bit_precision) {
				warn << "j: " << j << ", bit diff: " << diff;

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
