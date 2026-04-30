#include "convert.h"

/**
 * /file
 *
 * Support routines for converting lstm vector/matrix to qpu
 */

namespace {

bool check_precision(float lhs, float rhs, float precision) {
	if (precision == 0.0f) {
		if (lhs != rhs) {
			warn << "check_precision fail";
			return false;
		}
	} else {
		float diff = abs(lhs - rhs);
		if (diff > precision) {
			warn << "check_precision fails with "
			     << "diff: " << diff << " for precision: " << precision;
			return false;
		}
	}

	return true;
}

} // anon namespace

/**
 * Ensure that size is a multiple of 16
 */
int resize(std::size_t in_size, bool do_dump) {
	int size = (int) in_size;

	if ((size & 0xf) != 0) {
		size = (size + 15) & ~0xf;

		if (do_dump) {
			warn << "resize(): size " << in_size << " passed in,  adjusting to multiple of 16"
				   << " -> " << size;
		}
	}

	return size;
}


qpu::vector copy(lstm::vector const &rhs) {
	int size = resize(rhs.size());
	qpu::vector ret(size);
	ret.set(0.0f);

	for (int i = 0; i < (int) rhs.size(); ++i) {
		ret[i] = rhs[i];
	}

	return ret;
}


qpu::matrix copy(lstm::matrix const &rhs) {
	int height = resize(rhs.size());
	assert(rhs.size() >= 1);
	auto width = resize(rhs[0].size());

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
	int size = resize(x.size() + y.size());
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


bool same(qpu::vector const &lhs, qpu::vector const &rhs, float precision) {
	for (int i = 0; i < (int) rhs.size(); ++i) {
		if (!check_precision(lhs[i], rhs[i], precision)) {
			warn << "Fail same(qpu::vector, qpu::vector), index: " << i;
			return false;
		}
	}

	return true;
}


bool same(qpu::matrix const &lhs, lstm::matrix const &rhs, float precision) {
	assert(rhs.size() >= 1);
	auto width = rhs[0].size();

  for (std::size_t i = 0; i < rhs.size(); i++) {
    for (std::size_t j = 0; j < width; j++) {
			if (!check_precision(lhs.at((int) i, (int) j), rhs[i][j], precision)) {
				warn << "Fail same(qpu::matrix, qpu::matrix), (i,j): (" << i << ", " << j << ")";
				return false;
			}
    }
  }

	return true;
}


