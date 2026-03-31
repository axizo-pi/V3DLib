#include "V3DLib.h"
#include "Support/Settings.h"
#include "vc4/RegisterMap.h"
#include "vc4/PerformanceCounters.h"

using namespace V3DLib;
using namespace Log;

V3DLib::Settings settings;


void hello(Int::Ptr p) {                          // The kernel definition
	*p = 1;
}


int main(int argc, const char *argv[]) {
/*	
  warn << RegisterMap::ProgramRequestStatus();
  warn << "PerformanceCounters::enabled(): " << vc4::PerformanceCounters::enabled();
  warn << "TechnologyVersion(): " << RegisterMap::TechnologyVersion();
*/
  settings.init(argc, argv);

  auto k = compile(hello, settings);              // Construct the kernel

  Int::Array array(16);                           // Allocate and initialise the array shared between ARM and GPU
  array.fill(100);

  k.load(&array).run();                           // Invoke the kernel

  for (int i = 0; i < (int) array.size(); i++) {  // Display the result
    printf("%i: %i\n", i, array[i]);
  }

  return 0;
}
