#include "Kernels.h"
#include "V3DLib.h"

namespace {

using namespace V3DLib;

/**
 * Common part of the QPU kernels
 */
void mandelbrotCore(Complex const &c, Int &numIterations, Int &count) {
  count = 0;
  Complex x = c;
  Float mag = x.mag_square(); // Putting this in condition doesn't work

  // Following is a float version of boolean expression: ((reSquare + imSquare) < 4 && count < numIterations)
  // It works because `count` increments monotonically.
  FloatExpr condition = (4.0f - mag)*toFloat(numIterations - count);
  Float checkvar = condition;

/*
  Examination of reason why kernel does not do complete output.

  My best hypothesis for now is that there is a maximum number of instructions
  that can be executed per kernel call.

  This doesn't make sense, because Gravity can run indefinitely; perhaps there 
  are other conditions.

  vc7: Looking a performance counters here doesn't help; they don't appear to be updated (TODO)

  vc6 1 QPU
  ---------
  i < 1024 : black
  i <  512 : black
  i <  256 : black
  i <  128 : about 25% done
  i <   96 : about 45% done
  i <   64 : about 66% done
  i <   56 : about 75% done
  i <   48 : full when eyeballing
  i <   32 : full when eyeballing

   - vc7 not same but comparable
* /
  For (Int i = 0, i < 256, i++)
    //Where((4.0f > mag) && (numIterations > count))
    Where (checkvar > 0.0f)
      count++;

      x        = x*x + c;
      mag      = x.mag_square();
      checkvar = condition; 
    End
  End
  */
  

  While (any(checkvar > 0.0f))
    Where (checkvar > 0.0f)
      x = x*x + c;

      mag = x.mag_square();
      count++;
      checkvar = condition; 
    End
  End
}

} // anon namespace

namespace V3DLib {


/**
 * @brief Multi-QPU version
 */
void mandelbrot_multi(
  Float topLeftReal, Float topLeftIm,
  Float offsetX, Float offsetY,
  Int numStepsWidth, Int numStepsHeight,
  Int numIterations,
  Int::Ptr result,
  Int num_repeats
) {
  Int yMax = numStepsHeight - numQPUs();

  For (Int c = 0, c < num_repeats, c++)
    For (Int yIndex = me(), yIndex < yMax, yIndex += numQPUs())
      Int::Ptr dst = result + yIndex*numStepsWidth;

      For (Int xStep = 0, xStep < numStepsWidth, xStep += 16)
        Int xIndex = xStep + index();
        Int count;
        Complex c(topLeftReal + offsetX*toFloat(xIndex), topLeftIm - offsetY*toFloat(yIndex));

        mandelbrotCore(c, numIterations, count);

        *dst = count;
        dst.inc();
      End
    End
  End
}
} // namespace V3DLib
