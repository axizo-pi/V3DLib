#include <V3DLib.h>
#include "Support/Settings.h"

using namespace V3DLib;

V3DLib::Settings settings;

void id_kernel(Int::Ptr p, Int::Ptr q) {
  p += 16*me();
  q += 16*me();

  *p = me();
  *q = index();
}


int main(int argc, const char *argv[]) {
  printf("Compiling for vc7: %d\n", Platform::compiling_for_vc7());

  int numQPUs = 8;                                // Max number of QPUs for v3d

  settings.init(argc, argv);

  auto k = compile(id_kernel, settings);          // Construct kernel
  k.setNumQPUs(numQPUs);

  Int::Array result(16*numQPUs);                  // Allocate and initialise array shared between ARM and GPU
  result.fill(-1);

  Int::Array index_array(16*numQPUs);
  index_array.fill(2);

  k.load(&result, &index_array).run();            // Load the uniforms and invoke the kernel

  for (int i = 0; i < (int) result.size(); i++) { // Display the result
    printf("%3i: %2i, %2i\n", i, result[i], index_array[i]);
  }
  
  return 0;
}
