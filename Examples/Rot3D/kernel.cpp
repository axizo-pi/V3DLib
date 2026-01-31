#include "kernel.h"
#include "settings.h"
#include "Support/Timer.h"
#include <V3DLib.h>
#include <math.h>            // sinf(), cosf()

using namespace kernels;
using namespace V3DLib;

namespace {

// ============================================================================
// Local functions
// ============================================================================

template<typename Arr>
void init_arrays(Arr &x, Arr &y) {
  for (int i = 0; i < settings.num_vertices; i++) {
    x[i] = (float) i;
    y[i] = (float) i;
  }
}


template<typename Arr>
void disp_arrays(Arr &x, Arr &y) {
  if (settings.show_results) {
    for (int i = 0; i < settings.num_vertices; i++)
      printf("%f %f\n", x[i], y[i]);
  }
}

} // anon namespace


void run_qpu_kernel(KernelType &kernel) {
  auto k = compile(kernel, settings);  // Construct kernel
  k.setNumQPUs(settings.num_qpus);

  // Allocate and initialise arrays shared between ARM and GPU
  Float::Array x(settings.num_vertices), y(settings.num_vertices);
  init_arrays(x, y);

  Timer timer;
  k.load(settings.num_vertices, cosf(settings.THETA), sinf(settings.THETA), &x, &y).run();
  timer.end(!settings.silent);

  disp_arrays(x, y);
}


void run_qpu_kernel_3() {
  auto k = compile(rot3D_3_decorator(settings.num_vertices, settings.num_qpus), settings);
  k.setNumQPUs(settings.num_qpus);

  // Allocate and initialise arrays shared between ARM and GPU
  Float::Array x(settings.num_vertices), y(settings.num_vertices);
  init_arrays(x, y);

  Timer timer;
  k.load(cosf(settings.THETA), sinf(settings.THETA), &x, &y).run();
  timer.end(!settings.silent);

  disp_arrays(x, y);
}
