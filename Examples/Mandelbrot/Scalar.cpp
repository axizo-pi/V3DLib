#include "Scalar.h"
#include "Settings.h"


/**
 * Scalar kernel
 *
 * This runs on the CPU
 */
void mandelbrot_cpu(int *result) {
  for (int xStep = 0; xStep < settings().numStepsWidth; xStep++) {
    for (int yStep = 0; yStep < settings().numStepsHeight; yStep++) {
      float realC = settings().topLeftReal   + ((float) xStep)*settings().offsetX();
      float imC   = settings().bottomRightIm + ((float) yStep)*settings().offsetY();

      int count = 0;
      float real = realC;
      float im   = imC;
      float radius = (real*real + im*im);

      while (radius < 4 && count < settings().num_iterations) {
        float tmpReal = real*real - im*im;
        float tmpIm   = 2*real*im;
        real = tmpReal + realC;
        im   = tmpIm + imC;

        radius = (real*real + im*im);
        count++;
      }

      result[xStep + yStep*settings().numStepsWidth] = count;
    }
  }
}
