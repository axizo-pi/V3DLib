#include "doctest.h"
#include <V3DLib.h>
#include "../Lib/vc4/RegisterMap.h"
#include "../Lib/global/log.h"
#include "Support/Helpers.h"

using namespace Log;
using namespace V3DLib;

#ifdef QPU_MODE

namespace {

void mutex_kernel(Int::Ptr ret) {
  Int val = me();

  mutex_acquire();      comment("mutex_acquire");
  *ret = val;
  mutex_release();      comment("mutex_release");
}


} // anon namespace

TEST_CASE("Test mutexes[mutex]") {
  if (!Platform::compiling_for_vc4()) {
    warn << "Doing mutexes only for vc4";
    return;
  }

  RegisterMap::L2Cache_enable(false);
  warn << "L2CacheEnabled(): " << RegisterMap::L2CacheEnabled();

  SUBCASE("Test acquire/release") {
    int numQPUs = 1;
    Int::Array result(16);

    auto k = compile(mutex_kernel);
		to_file("mutex_kernel.txt", k.dump());
    k.load(&result);
    k.setNumQPUs(numQPUs);
    //k.run();
  }

  RegisterMap::L2Cache_enable(true);
  warn << "L2CacheEnabled(): " << RegisterMap::L2CacheEnabled();
}

#endif  // QPU_MODE
