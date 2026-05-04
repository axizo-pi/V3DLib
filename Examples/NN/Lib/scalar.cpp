#include "scalar.h"
#include "global/log.h"
#include <cmath>  // exp()

namespace scalar {

/**
 * activation function
 */
float sigmoid(float x) {
	return (float) (1.0/(1.0 + std::exp(-x)));
}

} // namespace scalar
