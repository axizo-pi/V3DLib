#include "settings.h"
#include "global/Timer.h"
#include "scalar.h"
#include "kernel.h"

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

  Timer timer("Total time", !settings.silent);

  switch (settings.kernel) {
    case 0: run_kernel();  break;  
    case 1: run_scalar(); break;
  }

  if (!settings.silent) {
    printf("Ran kernel '%s' with %d QPU's\n", settings.kernel_name.c_str(), settings.num_qpus);
  }

  return 0;
}
