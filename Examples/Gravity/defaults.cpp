#include "defaults.h"
#include "Support/Platform.h"
#include "global/log.h"

using namespace V3DLib;

int batch_steps() {
  if (Platform::compiling_for_vc4()) {
    //Log::warn << "batch_steps(): always returning 1 for vc4";
    return 1;
  } else {
    return BATCH_STEPS;
  }
}
