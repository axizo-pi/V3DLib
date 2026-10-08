#include "defaults.h"
#include "Support/Platform.h"
#include "global/log.h"

using namespace V3DLib;

double const YEAR   = 86400 * 365;               // approximately a year in seconds
double const DECADE = 86400 * 365 * 10;          // approximately a decade in seconds

double t_end = ((double) 25) * DECADE;

void set_num_years(int val) {
	//Log::warn << "set_num_years val: " << val;
	assert(val > 0);
  t_end = ((double) val) * YEAR;
}

int batch_steps() {
  if (Platform::running::vc4()) {
    return 1;
  } else {
    return BATCH_STEPS;
  }
}
