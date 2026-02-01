#include "settings.h"
#include "scalar.h"
#include "kernel.h"
#include "stlfile.h"

using namespace std;
using namespace kernels;


/**
 * Run a kernel as specified by the passed kernel index
 */
void run_kernel(int kernel_index) {
  switch (kernel_index) {
    case 0: run_qpu_kernel(rot3D_2);  break;  
    case 1: run_qpu_kernel_3();       break;  
    case 2: run_qpu_kernel(rot3D_1);  break;  
    case 3: run_qpu_kernel(rot3D_1a); break;  
    case 4: run_scalar_kernel();      break;
  }

  auto name = kernel_id[kernel_index];

  if (!settings.silent) {
    cout << "Ran kernel '" << name << "' "
		     << "with " << settings.num_qpus << " QPU's.\n";
  }
}


// ============================================================================
// Main
// ============================================================================

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);
	if (settings.show_info) return 0;

	load_stl(settings.stl_file);
 	run_kernel(settings.kernel);

  return 0;
}
