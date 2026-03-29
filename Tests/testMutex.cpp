#include "doctest.h"
#include <V3DLib.h>
#include "vc4/RegisterMap.h"
#include "LibSettings.h"
#include "global/log.h"
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
 * @brief add waiting NOP sequences if necessary.
 *
 * Revelation: DMA on Zero takes longer than on Pi3. Same applies to Pi1.
 * Following adjusts wait time for these cases.
 */
void add_nop() {
  if (Platform::use_main_memory()) return; // Compiling for emulator, don't bother

  switch (Platform::tag()) {
    case Platform::pi1:     nop(10); break;
    case Platform::pi2:              break;
    case Platform::pi_zero: nop(20); break;
    default: /* Don't bother */      break;
  }
}


/**
 * Assumption: While-loop not working properly
 *
 * This kernel investigates the While-loop.
 */
void while_kernel(Int::Ptr ret) {
  Int count = 0;
  Int condition = 0;

  While (condition == 0)
    // write/read before setting condition
    *ret = count;

    add_nop();

    Int tmp = *ret;

    If (tmp == 4)
      condition = 1;  comment("Set condition");
    Else
      condition = 0;
    End

    count++;
  End
}


/**
 * @brief Test consecutive store/load under various circumstances.
 *
 * This kernel does *NOT* work for `vc4` with TMU load enabled.
 * Unclear why, I gave up examining.
 * The hypothesis is that the TMU load does not get the most recent value.
 */
void for_kernel(Int::Ptr ret) {
  Int count = 0;
  Int condition = 0;

  For (count = 0, (count < 10 && condition == 0), count++)
    // The issue here is that the store and load follow very closely.
    // It is possible (and actually happens here) that the store is not done
    // when the load is executed.
    *ret = count;

    add_nop();

    Int tmp = *ret;

    If (tmp == 4)
      condition = 1;  comment("Set condition");
    Else
      condition = 0;
    End
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

/*
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
*/
}


/**
 * @brief Run barrier on emulator
 *
 * This is specifically meant for the `vc4` implementation of `barrier`,
 * but should work on `v3d` as well.
 */
TEST_CASE("Test barrier[mutex][barrier]") {
  LibSettings::tmu_load tmu(false);

  int numQPUs = 1;

  SUBCASE("Test barrier emulator") {
    Platform::main_mem mem(true);

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

    //warn << "result: " << result.dump();
    REQUIRE(result == expected);

    INFO("Multiple QPU's");
    numQPUs = 8;
    init_arrays(result, expected, signal, numQPUs);
    k.load(&result, &signal);
    k.setNumQPUs(numQPUs);
    k.run();

    //warn << "result:\n" << result.dump();
    //warn << "signal:\n" << signal.dump();
    REQUIRE(result == expected);

    Platform::use_main_memory(false);
  }

  SUBCASE("Test barrier QPU") {
    Int::Array result(16);
    Int::Array expected(16);

    // Last vector signals that a barrier leader is present
    Int::Array signal(16*(numQPUs + 1));
    signal.fill(0);

    auto k = compile(barrier_kernel);

    INFO("Single QPU");
    numQPUs = 1;
    init_arrays(result, expected, signal, numQPUs);
    k.load(&result, &signal);
    k.setNumQPUs(numQPUs);
    k.run();

    //warn << "result: " << result.dump();
    REQUIRE(result == expected);

/*
    INFO("Multiple QPU's");
    numQPUs = 2;
    init_arrays(result, expected, signal, numQPUs);
    k.load(&result, &signal);
    k.setNumQPUs(numQPUs);
    k.run();

    warn << "result:\n" << result.dump();
    warn << "signal:\n" << signal.dump();
    REQUIRE(result == expected);
*/
  }
}


TEST_CASE("Test While-loop emulator[mutex][while]") {
  LibSettings::tmu_load tmu(false);

  SUBCASE("Test Emulator") {
    Platform::use_main_memory(true);

    int numQPUs = 1;
    Int::Array result(16);
    result.fill(-1);

    Int::Array expected(16);
    expected.fill(4);

    auto k = compile(while_kernel);
    to_file("while_kernel.txt", k.dump());
    k.setNumQPUs(numQPUs);
    k.load(&result);
    k.run();

    //warn << "result While: " << result.dump();
    REQUIRE(result == expected);

    Platform::use_main_memory(false);
  }

  // Fails with mailbox error with TMU load, 
  // Works fine with DMA load.
  SUBCASE("Test QPU") {
    int numQPUs = 1;
    Int::Array result(16);
    result.fill(-1);

    Int::Array expected(16);
    expected.fill(4);

    auto k = compile(while_kernel);
    k.setNumQPUs(numQPUs);
    to_file("while_kernel_2.txt", k.dump());
    k.load(&result);
    k.run();

    //warn << "result While2: " << result.dump();
    REQUIRE(result == expected);
  }
}


/**
 * This tests issues with load/store timing.
 * The trigger was an off-by-one error in the emulator.
 *
 * Result contained 5 instead of 4.
 * Problem fixed, these tests are for regression.
 *
 * Not bothering with testing TMU load, doesn't work for
 * for QPU and is off-by-1 one for emulator.
 */
TEST_CASE("Test For-loop[mutex][for]") {
  LibSettings::tmu_load tmu(false);

  SUBCASE("Emulator with DMA load") {
    Platform::main_mem mem(true);

    int numQPUs = 1;
    Int::Array result(16);
    result.fill(-1);

    Int::Array expected(16);
    expected.fill(4);

    auto k = compile(for_kernel);
    to_file("for_kernel.txt", k.dump());
    k.load(&result);
    k.setNumQPUs(numQPUs);
    k.emu();

    //warn << "result For: " << result.dump();
    REQUIRE(result == expected);
  }

  SUBCASE("QPU with DMA load") {
    int numQPUs = 1;
    Int::Array result(16);
    result.fill(-1);

    Int::Array expected(16);
    expected.fill(4);

    auto k = compile(for_kernel);
    k.setNumQPUs(numQPUs);
    //to_file("for_kernel_2.txt", k.dump());
    k.load(&result);
    k.run();

    //warn << "result For2: " << result.dump();
    REQUIRE(result == expected);
  }
}
