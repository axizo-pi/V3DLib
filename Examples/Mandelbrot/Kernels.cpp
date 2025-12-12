#include "Kernels.h"
#include "V3DLib.h"

namespace {

using namespace V3DLib;

/**
 * Common part of the QPU kernels
 */
void mandelbrotCore(Complex const &c, Int &numIterations, Int::Ptr &dst) {
  Int count = 0;
  Complex x = c;
  Float mag = x.mag_square(); // Putting this in condition doesn't work

  // Following is a float version of boolean expression: ((reSquare + imSquare) < 4 && count < numIterations)
  // It works because `count` increments monotonically.
  FloatExpr condition = (4.0f - mag)*toFloat(numIterations - count);
  Float checkvar = condition;

	/////////////////////////////////////
	// Succeeds - all with -dim=768
	/////////////////////////////////////
/*	
  For (Int i = 0, i < 128 , i++)  // als max = 64
  End

  For (Int i = 0, i < 128, i++)
    Where (checkvar > 0.5f)
      count++;
    End
  End
*/		

	/////////////////////////////////////
	// Partial success
	/////////////////////////////////////
/*
	// Default kernel is also partial success

  For (Int i = 0, i < 1024, i++)  // also max = 512
  End

  For (Int i = 0, i < 128, i++)
    Where (checkvar > 0.5f)
      x = x*x + c;

      mag = x.mag_square();
      count++;
      checkvar = condition; 
    End
  End
*/	

	/////////////////////////////////////
	// Fails 
	/////////////////////////////////////
/*	
  For (Int i = 0, i < 128, i++)
      count++;
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

	
  *dst = count;
}

} // anon namespace

namespace V3DLib {
void mandelbrot_single(
  Float topLeftReal, Float topLeftIm,
  Float offsetX, Float offsetY,
  Int numStepsWidth, Int numStepsHeight,
  Int numIterations,
  Int::Ptr result
) {
  For (Int yStep = 0, yStep < numStepsHeight, yStep++)
    Int::Ptr dst = result + yStep*numStepsWidth;
		Int xMax = (numStepsWidth - 16);

    For (Int xStep = 0, xStep < xMax, xStep += 16)
      Int xIndex = xStep + index();

      mandelbrotCore(
        Complex(topLeftReal + offsetX*toFloat(xIndex), topLeftIm - offsetY*toFloat(yStep)),
        numIterations,
        dst);

      dst.inc();  comment("dst increment");
    End

    //Int::Ptr dst2 = result + yStep*numStepsWidth;
		//*dst2 = 128;
  End

  Int::Ptr dst3 = result + (numStepsHeight/2)*numStepsWidth;
	*dst3 = 512;
}


/**
 * @brief Multi-QPU version
 */
void mandelbrot_multi(
  Float topLeftReal, Float topLeftIm,
  Float offsetX, Float offsetY,
  Int numStepsWidth, Int numStepsHeight,
  Int numIterations,
  Int::Ptr result
) {
  Int yMax = numStepsHeight - numQPUs();

  For (Int yStep = 0, yStep < yMax, yStep += numQPUs())
    Int yIndex = yStep + me();
    Int::Ptr dst = result + yIndex*numStepsWidth;

    For (Int xStep = 0, xStep < numStepsWidth - 16, xStep += 16)
      Int xIndex = xStep + index();

      mandelbrotCore(
        Complex(topLeftReal + offsetX*toFloat(xIndex), topLeftIm - offsetY*toFloat(yIndex)),
        numIterations,
        dst);

      dst.inc();
    End
  End
}
} // namespace V3DLib
