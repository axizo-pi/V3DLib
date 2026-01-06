#include "Scalar.h"
#include "Settings.h"


/**
 * Scalar kernel
 *
 * This runs on the CPU
 */
void mandelbrot_cpu(int *result) {
	auto const &s = settings();

  for (int xStep = 0; xStep < s.numStepsWidth; xStep++) {
    for (int yStep = 0; yStep < s.numStepsHeight; yStep++) {
      float realC = s.topLeftReal   + ((float) xStep)*s.offsetX();
      float imC   = s.bottomRightIm + ((float) yStep)*s.offsetY();

      int count = 0;
      float real = realC;
      float im   = imC;
      float radius = (real*real + im*im);

      while (radius < 4 && count < s.num_iterations) {
        float tmpReal = real*real - im*im;
        float tmpIm   = 2*real*im;
        real = tmpReal + realC;
        im   = tmpIm + imC;

        radius = (real*real + im*im);
        count++;
      }

      result[xStep + yStep*s.numStepsWidth] = count;
    }
  }
}
