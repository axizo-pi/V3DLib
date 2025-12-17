#ifndef _INCLUDE_MANDELBROT_KERNELS
#define _INCLUDE_MANDELBROT_KERNELS
#include "Source/Complex.h"

namespace V3DLib {

void mandelbrot_multi(
  Float topLeftReal, Float topLeftIm,
  Float offsetX, Float offsetY,
  Int numStepsWidth, Int numStepsHeight,
  Int numIterations,
  Int::Ptr result
);

} // namespace V3DLib

#endif // _INCLUDE_MANDELBROT_KERNELS
