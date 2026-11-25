#include "Settings.h"
#include <V3DLib.h>
#include "Scalar.h"
#include "Kernels.h"
#include "animate.h"
#include "Support/bmp.h"
#include "global/Timer.h"

#include "Mandelbrot_hex_kernels.h"

using namespace V3DLib;
using namespace Log;


// ============================================================================
// Local functions
// ============================================================================

template<class Array>
void output_image(Array &result, std::string filename = "") {
  int width         = settings().numStepsWidth;
  int height        = settings().numStepsHeight;
  int numIterations = settings().num_iterations;

  if (settings().output_grey) {
    if (filename.empty()) {
      filename = "mandelbrot.bmp";
    }
    output_bmp(result, width, height, numIterations, filename.c_str(), false);
  }

  if (settings().output_color) {
    if (filename.empty()) {
      filename = "mandelbrot_c.bmp";
    }
    output_bmp(result, width, height, numIterations, filename.c_str(), true);
  }
}


BaseKernel prepare_kernel() {
  auto const &s = settings();

  //warn << "Run precompiled kernels: " << s.runPrecompKernels; 
  if (s.runPrecompKernels) {
    return load_kernel(kernels, s);
  } else {
    return compile(mandelbrot_kernel, s);
  }
}

void run_qpu_kernel() {
  auto const &s = settings();

  assertq(0 == s.numStepsWidth % 16, "Width dimension must be a multiple of 16");
  assertq((settings().run_type != 0) || !Platform::hw::vc4() || (4 <= s.num_qpus),
          "Num QPU's must be at least 4 for vc4");

  std::string timer_label = (s.runPrecompKernels)?"Kernel load_kernel":"Kernel compile";

  Timer timer(timer_label);
  auto k = prepare_kernel();
  warn << timer_label << ": " << timer.dump();

  if (s.compile_only) return;

  k.setNumQPUs(s.num_qpus);

  Int::Array result(s.num_items());  // Allocate and initialise

  k.load(
    s.topLeftReal, s.topLeftIm,
    s.offsetX(), s.offsetY(),
    s.numStepsWidth, s.numStepsHeight,
    s.num_iterations,
    &result,
    s.count
  );
  
  Timer timer2("Kernel run");
  k.run();
  warn << "Kernel run: " << timer2.dump();

  output_image(result);
}


/**
 * @brief Run a kernel as specified by the selected kernel index
 */
void run_kernel() {
  int kernel_index = settings().kernel;

  switch (kernel_index) {
    case 0: run_qpu_kernel();  break;  
    case 1: {
        int *result = new int [settings().num_items()];  // Allocate and initialise

        mandelbrot_cpu(result);
        output_image(result);
        delete result;
      }
      break;
  }

  auto name = MandSettings::kernels()[kernel_index];

  if (!settings().silent) {
    printf("Ran kernel '%s' with %d QPU's\n", name, settings().num_qpus);
  }
}


// ============================================================================
// Main
// ============================================================================

int main(int argc, const char *argv[]) {
  settings_init(argc, argv);
  run_kernel();

  printf("\n");
  return 0;
}
