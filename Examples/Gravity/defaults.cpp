#include "defaults.h"
#include "Support/Platform.h"
#include "global/log.h"

using namespace V3DLib;

int batch_steps() {
  if (Platform::compile::for_vc4()) {
    return 1;
  } else {
    return BATCH_STEPS;
  }
}
