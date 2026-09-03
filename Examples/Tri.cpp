#include "V3DLib.h"
#include <CmdParameters.h>
#include "Support/Settings.h"
#include <iostream>

using namespace V3DLib;

Settings settings("Tri - Calculate triangular numbers\n", true);


void tri_kernel(Int::Ptr p, Int::Ptr result) {
  p += me()*16;
  result += me()*16;

  Int n = *p;
  Int sum = 0;

  While (any(n > 0))
    Where (n > 0)
      sum = sum + n;
      n = n - 1;
    End
  End

  *result = sum;
}


void run_kernel() {
  // Allocate and initialise arrays shared between ARM and GPU
  Int::Array array(settings.num_qpus*16);
  for (int i = 0; i < (int) array.size(); i++)
    array[i] = i;

  Int::Array result(settings.num_qpus*16);

  // Construct kernel
  auto k = compile(tri_kernel, settings);
  k.setNumQPUs(settings.num_qpus);

  // Invoke the kernel
  k.load(&array, &result).run();

  // Display the result
  for (int i = 0; i < (int) result.size(); i++)
    printf("%i: %i\n", i, result[i]);
}


///////////////////////////////////////////
// Main
///////////////////////////////////////////

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

  run_kernel();
  return 0;
}
