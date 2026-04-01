#include "Settings.h"
#include "Scalar.h"
#include "Kernels.h"
#include "animate.h"
#include "V3DLib.h"
#include <string>
#include "Support/Timer.h"
#include "Support/bmp.h"

using namespace V3DLib;
using namespace Log;
using std::string;


// ============================================================================
// Local functions
// ============================================================================

using KernelType = decltype(mandelbrot_multi);

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


void run_qpu_kernel(KernelType &kernel) {
	const bool do_animate = false;
	auto const &s = settings();

  assertq(0 == s.numStepsWidth % 16, "Width dimension must be a multiple of 16");

	//
	// Allowed to run if:
	// - v3d QPU
	// - vc4 interpreter or emulator
	// - vc4 QPU with #QPU's >= 0
	//
	// TODO recheck following condition
	//
  assertq(!Platform::compiling_for_vc4() || (settings().run_type != 0) || (4 <= s.num_qpus),
    "Num QPU's must be at least 4 for vc4"
  );

  Timer timer("Kernel compile");
  auto k = compile(kernel, s);
  timer.end(!s.silent);

  k.setNumQPUs(s.num_qpus);

  Int::Array result(s.num_items());  // Allocate and initialise

	if (do_animate) {
		int index = 0;
		animate::init(s);

		while (!animate::done()) {
			MandRange r = animate::next();
			//Log::warn << "MandRange r:\n" << r.dump();
			if (index % 100 == 0) {
				warn << "index: " << index;
			}

			std::string filename;
			filename << index << "_mandelbrot.bmp";

	  	k.load(
		    r.topLeftReal, r.topLeftIm,
		    r.offsetX(), r.offsetY(),
		    r.numStepsWidth, r.numStepsHeight,
		    s.num_iterations,
		    &result,
		    s.count
		  ).run();
  
			output_image(result, filename);
			index++;
		}
	} else {
	  k.load(
	    s.topLeftReal, s.topLeftIm,
	    s.offsetX(), s.offsetY(),
	    s.numStepsWidth, s.numStepsHeight,
	    s.num_iterations,
	    &result,
	    s.count
	  ).run();
	}

  output_image(result);
}


/**
 * Run a kernel as specified by the passed kernel index
 */
void run_kernel(int kernel_index) {
  Timer timer;

  switch (kernel_index) {
    case 0: run_qpu_kernel(mandelbrot_multi);  break;  
    case 1: {
        int *result = new int [settings().num_items()];  // Allocate and initialise

        mandelbrot_cpu(result);
        output_image(result);

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
