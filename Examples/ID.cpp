#include <V3DLib.h>
#include "Support/Settings.h"
#include "Support/Helpers.h"  // to_file()

#ifdef USE_HEX_KERNEL
#include "../obj/debug/generated/ID_hex_kernels.cpp"
#endif    

using namespace V3DLib;

V3DLib::Settings settings;

void id_kernel(Int::Ptr p, Int::Ptr q) {
  p += 16*me();
  q += 16*me();

  *p = me();
  *q = index();
}

int main(int argc, const char *argv[]) {
  int numQPUs = 8;                                // Max number of QPUs for vc6, can be increased for vc4 and vc7
  settings.init(argc, argv);

#ifdef USE_HEX_KERNEL
  Log::warn << "App version with precompiled headers.";
  auto k = load_kernel(kernels, settings);
#else
  auto k = compile(id_kernel, settings);          // Construct kernel
#endif
  if (settings.compile_only) return 0;

  k.setNumQPUs(numQPUs);

   // Allocate and initialise array shared between ARM and GPU
  Int::Array result(16*numQPUs);                  // QPU index
  result.fill(-1);

  Int::Array index_array(16*numQPUs);             // register (vector) index
  index_array.fill(2);

  k.load(&result, &index_array).run();            // Load the uniforms and invoke the kernel

  for (int i = 0; i < (int) result.size(); i++) { // Display the result
    printf("%3i: %2i, %2i\n", i, result[i], index_array[i]);
  }

  return 0;
}
