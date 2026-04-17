#include "settings.h"
#include "scalar.h"
#include "kernel.h"
#include "stlfile.h"
#include "LibSettings.h"

using namespace std;
using namespace kernels;


/**
 * Run a kernel as specified by the passed kernel index
 */
void run_kernel(int kernel_index) {
  switch (kernel_index) {
    case 0: run_qpu_kernel(vector_rot3D);  break;  
    case 1: run_scalar_kernel();      break;
    default: assert(false);
  }

  settings.show_run_info();
}


// ============================================================================
// Main
// ============================================================================

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);
  if (settings.show_info) return 0;
  V3DLib::LibSettings::use_high_precision_sincos(true);

  load_stl(settings.stl_file);
  run_kernel(settings.kernel);

  return 0;
}
