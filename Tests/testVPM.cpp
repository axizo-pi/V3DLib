#include "doctest.h"
#include <V3DLib.h>
#include "vc4/DMA/VPMArray.h"
#include "global/log.h"
#include "LibSettings.h"
#include "vc4/DMA/Operations.h"
#include "Support/Helpers.h"

using namespace Log;
using namespace V3DLib;

namespace {

IntExpr next_addres() {
  Int tmp2 = (me() + 1);  comment("Read value from next QPU");
  Where (tmp2 == numQPUs())
    tmp2 = 0;
  End

  return tmp2;
}


/**
 * @brief Test using VPM as shared memory
 *
 * VPM is used to transfer values between QPU's.
 *
 * The issue here is that it take a non-zero time to fill the VPM.
 * You need to add a delay between storing and retrieving.
 *
 * VPM is vc4-only.
 */
void vpm_kernel(Int::Ptr ret) {
  Int tmp = 10*(me() + 1);

  // Write own value to VPM
  vpmSetupWrite(HORIZ, me());
  vpmPut(tmp);

  VPMArray::add_nop(); // TODO: Check if calling this just before vpmGetInt() makes a difference

  // Read other value from VPM
  Int tmp2 = next_addres();

  vpmSetupRead(HORIZ, 1, tmp2);
  Int tmp3 = vpmGetInt();

  *(ret + 16*me()) = tmp3;
}


void vpm_array_kernel(Int::Ptr ret) {
  Int val = 10*(me() + 1);

  vpm.set(me(), val);                 // Write own value to VPM
  Int tmp3 = vpm.get(next_addres());  // Read other value from VPM

  *(ret + 16*me()) = tmp3;            // Store to main mem
}


void init_expected(Int::Array &expected, int numQPUs) {
  for (int i = 0; i < numQPUs; ++i) {
    for (int j = 0; j < 16; ++j) {
      expected[16*((i + (numQPUs - 1)) % numQPUs) + j] = 10*(i + 1);
    }
  }
}

}


/**
 * Test using VPM as shared memory for the QPU's.
 *
 * Note that VPM as mem and DMA will interfere in the VPM usage!
 * Just don't do DMA when using VPM as mem.
 */
TEST_CASE("Test VPM memory [vpm]") {
  LibSettings::tmu_load tmu(false);
  int numQPUs = 4;


  SUBCASE("In emulator") {
    // This works on v3d
    Platform::main_mem mem(true);

    Int::Array result(numQPUs*16);

    Int::Array expected(numQPUs*16);
    init_expected(expected, numQPUs);

    auto k = compile(vpm_kernel);
    k.setNumQPUs(numQPUs);
    k.load(&result);
    k.emu();

    //std::cout << result.dump() << "\n";
    //std::cout << expected.dump() << "\n";
    REQUIRE(result == expected);
  }


  SUBCASE("on QPUs") {
    if (!Platform::compiling_for_vc4()) {
      warn << "Not running VPM mem on QPU's for v3d";
      return;
    }

    Int::Array result(numQPUs*16);

    Int::Array expected(numQPUs*16);
    init_expected(expected, numQPUs);

    auto k = compile(vpm_kernel);
    //to_file("vpm_kernel.txt", k.dump());
    k.setNumQPUs(numQPUs);
    k.load(&result);
    k.run();

    //std::cout << result.dump() << "\n";
    //std::cout << expected.dump() << "\n";
    REQUIRE(result == expected);
  }
}


/**
 * Same test as previous using VPM array interface
 */
TEST_CASE("Test VPM array [vpm][arr]") {
  LibSettings::tmu_load tmu(false);
  int numQPUs = 4;

  SUBCASE("In emulator") {
    // This works on v3d
    Platform::main_mem mem(true);

    Int::Array result(numQPUs*16);

    Int::Array expected(numQPUs*16);
    init_expected(expected, numQPUs);

    auto k = compile(vpm_array_kernel);
    k.setNumQPUs(numQPUs);
    k.load(&result);
    k.emu();

    //std::cout << result.dump() << "\n";
    //std::cout << expected.dump() << "\n";
    REQUIRE(result == expected);
  }


  SUBCASE("on QPUs") {
    if (!Platform::compiling_for_vc4()) {
      warn << "Not running VPM mem on QPU's for v3d";
      return;
    }

    Int::Array result(numQPUs*16);

    Int::Array expected(numQPUs*16);
    init_expected(expected, numQPUs);

    auto k = compile(vpm_array_kernel);
    //to_file("vpm_array_kernel.txt", k.dump());
    k.setNumQPUs(numQPUs);
    k.load(&result);
    k.run();

    //std::cout << result.dump() << "\n";
    //std::cout << expected.dump() << "\n";
    REQUIRE(result == expected);
  }
}
