#include "doctest.h"
#include <V3DLib.h>
#include "../Lib/vc4/RegisterMap.h"
#include "../Lib/global/log.h"
#include "Support/Helpers.h"

using namespace Log;
using namespace V3DLib;

namespace {

void mutex_kernel(Int::Ptr ret) {
  Int val      = me();
  Int::Ptr dst = ret + (me()*16);

  mutex_acquire();      comment("mutex_acquire");
  *dst = val;
  mutex_release();      comment("mutex_release");
}


/**
 * Set QPU num to each 16-vector of the passed array
 */
void init_vector(Int::Array &arr) {
  for (int i = 0; i < (int) arr.size(); ++i) {
    arr[i] = i / 16;
  }
}


void run_mutex_kernel(Int::Array &result, Int::Array &expected, int numQPUs, BaseKernel &k) {
  result.alloc(numQPUs*16);
  result.fill(-1);

  expected.alloc(numQPUs*16);
  init_vector(expected);

  k.load(&result);
  k.setNumQPUs(numQPUs);
  k.run();

  warn << "result: " << result.dump();
  REQUIRE(result == expected);
}


} // anon namespace


/**
 * @brief Run mutexes on emulator
 *
 * Emulator should work fine on v3d, no need to block.
 */
TEST_CASE("Test mutexes emulator[mutex]") {
  Platform::use_main_memory(true);
  INFO("Unit test [mutex] using main memory");

  SUBCASE("Test acquire/release") {
    Int::Array result(16);
    Int::Array expected(16);

    auto k = compile(mutex_kernel);
    //to_file("mutex_kernel.txt", k.dump());

    INFO("Single QPU");
    run_mutex_kernel(result, expected, 1, k);

    INFO("Multiple QPU's");
    run_mutex_kernel(result, expected, 8, k);
  }

  Platform::use_main_memory(false);
}


#ifdef QPU_MODE

/**
 * @brief Run mutexes on hardware
 * /
TEST_CASE("Test mutexes QPU[mutex]") {
  if (!Platform::compiling_for_vc4()) {
    warn << "Doing mutexes only for vc4";
    return;
  }

  RegisterMap::L2Cache_enable(false);
  info << "L2CacheEnabled(): " << RegisterMap::L2CacheEnabled();

  SUBCASE("Test acquire/release") {
    Int::Array result(16);
    Int::Array expected(16);

    auto k = compile(mutex_kernel);
    to_file("mutex_kernel.txt", k.dump());

    INFO("Single QPU");
    run_mutex_kernel(result, expected, 1, k);

    //INFO("Multiple QPU's");
    //run_mutex_kernel(result, expected, 8, k);
  }

  RegisterMap::L2Cache_enable(true);
  info << "L2CacheEnabled(): " << RegisterMap::L2CacheEnabled();
}
*/

#endif  // QPU_MODE
