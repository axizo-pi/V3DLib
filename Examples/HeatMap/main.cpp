#include "settings.h"
#include "Support/Timer.h"
#include "scalar.h"
#include "kernel.h"

using namespace V3DLib;

// ============================================================================
// Main
// ============================================================================

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

  Timer timer("Total time");

  switch (settings.kernel) {
    case 0: run_kernel();  break;  
    case 1: run_scalar(); break;
  }

  if (!settings.silent) {
    printf("Ran kernel '%s' with %d QPU's\n", settings.kernel_name.c_str(), settings.num_qpus);
  }
  timer.end(!settings.silent);

  return 0;
}
