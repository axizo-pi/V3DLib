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


void barrier_kernel(Int::Ptr ret, Int::Ptr signal) {
  Int val      = me();
  Int::Ptr dst = ret + (me()*16);

  barrier(signal);      comment("barrier");
  *dst = val;
}


/**
 * Assumption: While-loop not working properly/
 *
 * This kernel investigates the While-loop.
 */
void while_kernel(Int::Ptr ret) {
  Int count = 0;
  Int condition = 0;

  While (condition == 0)
    // write/read before setting condition
    *ret = count;
    Int tmp = *ret;

    If (tmp == 10)
      condition = 1;  comment("Set condition");
    Else
      condition = 0;
    End

    count++;
  End
}


/**
 * Set QPU num to each 16-vector of the passed array
 */
void init_vector(Int::Array &arr) {
  for (int i = 0; i < (int) arr.size(); ++i) {
    arr[i] = i / 16;
  }
}


void init_arrays(Int::Array &result, Int::Array &expected, int numQPUs) {
  result.alloc(numQPUs*16);
  result.fill(-1);

  expected.alloc(numQPUs*16);
  init_vector(expected);
}


void init_arrays(Int::Array &result, Int::Array &expected, Int::Array &signal, int numQPUs) {
  init_arrays(result, expected, numQPUs);

  // Last vector signals that a barrier leader is present
  signal.alloc(16*(numQPUs + 1));
  signal.fill(0);
}

} // anon namespace


/**
 * @brief Run mutexes on emulator
 *
 * Emulator should work fine on v3d, no need to block.
 */
TEST_CASE("Test mutexes emulator[mutex]") {
  int numQPUs = 1;


  SUBCASE("Test emulator") {
    Platform::use_main_memory(true);
    INFO("Unit test [mutex] using main memory");
    Int::Array result(16);          // Needs to be here because main memory used
    Int::Array expected(16);        // idem

    auto k = compile(mutex_kernel); // idem
    //to_file("mutex_kernel.txt", k.dump());

    INFO("Single QPU");
    numQPUs = 1;
    init_arrays(result, expected, numQPUs);
    k.load(&result);
    k.setNumQPUs(numQPUs);
    k.run();
    REQUIRE(result == expected);

    INFO("Multiple QPU's");
    numQPUs = 8;
    init_arrays(result, expected, numQPUs);
    k.load(&result);
    k.setNumQPUs(numQPUs);
    k.run();
    REQUIRE(result == expected);

    Platform::use_main_memory(false);
  }

#ifdef QPU_MODE

  SUBCASE("Test QPU") {
    if (!Platform::compiling_for_vc4()) {
      warn << "Doing mutexes only for vc4";
    } if (true) {
      warn << "Skipping QPU mutex unit test for now";
    } else {
      Int::Array result(16);
      Int::Array expected(16);

      auto k = compile(mutex_kernel);

      INFO("Single QPU");
      init_arrays(result, expected, numQPUs);
      k.load(&result);
      k.setNumQPUs(numQPUs);
      k.run();
      REQUIRE(result == expected);

      INFO("Multiple QPU's");
      numQPUs = 8;
      init_arrays(result, expected, numQPUs);
      k.load(&result);
      k.setNumQPUs(numQPUs);
      k.run();
      REQUIRE(result == expected);
    }
  }

#endif  // QPU_MODE
}


/**
 * @brief Run barrier on emulator
 */
TEST_CASE("Test barrier emulator[mutex]") {
  Platform::use_main_memory(true);
  INFO("Unit test [mutex] using main memory");
  int numQPUs = 1;

  SUBCASE("Test barrier") {
    Int::Array result(16);
    Int::Array expected(16);

    // Last vector signals that a barrier leader is present
    Int::Array signal(16*(numQPUs + 1));
    signal.fill(0);

    auto k = compile(barrier_kernel);
    to_file("barrier_kernel.txt", k.dump());

    INFO("Single QPU");
    numQPUs = 1;
    init_arrays(result, expected, signal, numQPUs);
    k.load(&result, &signal);
    k.setNumQPUs(numQPUs);
    k.run();
    warn << "result: " << result.dump();
    REQUIRE(result == expected);

    INFO("Multiple QPU's");
    if (true) {
      warn << "Skipping multi-QPU barrier unit test for now";
    } else {
      numQPUs = 2;
      init_arrays(result, expected, signal, numQPUs);
      k.load(&result, &signal);
      k.setNumQPUs(numQPUs);
      k.run();

      warn << "result: " << result.dump();
      warn << "signal: " << signal.dump();
      REQUIRE(result == expected);
    }
  }

  Platform::use_main_memory(false);
}


TEST_CASE("Test While-loop emulator[mutex][while]") {
  SUBCASE("Test Emulator") {
    Platform::use_main_memory(true);

    int numQPUs = 1;
    Int::Array result(16);
    result.fill(-1);

    auto k = compile(while_kernel);
    to_file("while_kernel.txt", k.dump());
    k.load(&result);
    k.setNumQPUs(numQPUs);
    k.run();
    //k.interpret();
    warn << "result While: " << result.dump();

    Platform::use_main_memory(false);
  }

  SUBCASE("Test QPU") {
    int numQPUs = 1;
    Int::Array result(16);
    result.fill(-1);

    auto k = compile(while_kernel);
    k.load(&result);
    k.setNumQPUs(numQPUs);
    k.run();
    warn << "result While: " << result.dump();
  }
}
