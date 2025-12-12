#include "Settings.h"
#include "Scalar.h"
#include "Kernels.h"

#include "V3DLib.h"
#include <string>
#include "Support/Timer.h"
#include "Support/pgm.h"

using namespace V3DLib;
using std::string;


// ============================================================================
// Local functions
// ============================================================================

using KernelType = decltype(mandelbrot_single);

template<class Array>
void output_pgm(Array &result) {
  int width         = settings().numStepsWidth;
  int height        = settings().numStepsHeight;
  int numIterations = settings().num_iterations;

  if (settings().output_pgm) {
    output_pgm_file(result, width, height, numIterations, "mandelbrot.pgm");
  }
  if (settings().output_ppm) {
    output_ppm_file(result, width, height, numIterations, "mandelbrot.ppm");
  }
}


void run_qpu_kernel(KernelType &kernel) {
  assertq(0 == settings().numStepsWidth % 16, "Width dimension must be a multiple of 16");

  Timer timer("Kernel compile");

  auto k = compile(kernel, settings());

  timer.end(!settings().silent);

  k.setNumQPUs(settings().num_qpus);

  Int::Array result(settings().num_items());  // Allocate and initialise

  k.load(
    settings().topLeftReal, settings().topLeftIm,
    settings().offsetX(), settings().offsetY(),
    settings().numStepsWidth, settings().numStepsHeight,
    settings().num_iterations,
    &result).run();

  output_pgm(result);
}


/**
 * Run a kernel as specified by the passed kernel index
 */
void run_kernel(int kernel_index) {
  Timer timer;

  switch (kernel_index) {
    case 0: run_qpu_kernel(mandelbrot_multi);  break;  
    case 1: run_qpu_kernel(mandelbrot_single); break;
    case 2: {
        int *result = new int [settings().num_items()];  // Allocate and initialise

        mandelbrot_cpu(result);
        output_pgm(result);

        delete result;
      }
      break;
  }

  auto name = MandSettings::kernels()[kernel_index];

  timer.end(!settings().silent);

  if (!settings().silent) {
    printf("Ran kernel '%s' with %d QPU's\n", name, settings().num_qpus);
  }
}


// ============================================================================
// Main
// ============================================================================

int main(int argc, const char *argv[]) {
  //printf("Check pre\n");
  //RegisterMap::checkThreadErrors();   // TODO: See if it's useful to check this every time after a kernel has run

  settings_init(argc, argv);

#ifdef ARM32
  if (!Platform::run_vc4() && settings().kernel <= 1) {
    printf("\nWARNING: Mandelbrot will run *sometimes* on a Pi4 with 32-bit Raspbian when GPU kernels are used "
           "(-k=multi or -k=single).\n"
           "Running it has the potential to lock up your Pi. Please use with care.\n\n");
  }
#endif  // ARM32


  if (settings().kernel == settings().ALL) {
    for (int i = 0; i < settings().ALL; ++i ) {
      run_kernel(i);
    }
  } else {
    run_kernel(settings().kernel);
  }

  //printf("Check post\n");
  //RegisterMap::checkThreadErrors();

  printf("\n");
  return 0;
}
