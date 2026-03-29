#include "doctest.h"
#include <V3DLib.h>
#include "global/log.h"
#include "LibSettings.h"
#include "vc4/DMA/Operations.h"
#include "Support/Helpers.h"

using namespace Log;
using namespace V3DLib;

namespace {


/**
 * ==================================================
 * Examination
 * -----------
 *
 * vc4 only.
 *
 * * pi3
 *   + 2 QPU's, count:
 *     - 10: #1 filled, #0 sometimes
 *     - 30: all filled
 *   + 4 QPU's, count:
 *     - 70: all filled
 *     - 60: #0, #1, #3 filled
 *     - 50: #0, #1, #3 filled
 *     - 40: #0, #1, #3 filled
 *     - 30: #0, #3 filled (once all filled)
 *     - 20: #0, #3 filled
 *     - 10: only #3 filled
 *     -  0: only #3 filled
 *   + 8 QPU's, count:
 *     - 80: all filled except #2
 *     - 70: all filled except #2
 * * Pi2
 *   + 4 QPU's, count:
 *     - 100: all filled
 *     -  90: all filled, #2 fails sometimes
 * * Pi1
 *   + 4 QPU's, count:
 *     - 100: all filled
 *     -  90: all filled, #2 fails sporadically
 *     -  80: #0, #1, #3 filled, #2 fails
 * * zero
 *   + 4 QPU's, count:
 *     - 90: all filled
 *     - 80 #2 not filled, rest filled
 *     - 70 #2 not filled, rest filled
 */
void add_nop() {
  if (Platform::use_main_memory()) {
    // Compiling for emulator, don't bother
    return;
  }

  int count = 80;

  switch (Platform::tag()) {
    case Platform::pi1:     count = 100; break;
    case Platform::pi2:     count = 100; break;
    case Platform::pi_zero: count =  90; break;
    default:
      // Default
      break;
  }

  if (Platform::tag() == Platform::pi_zero) {
    count = 90;
  }

  nop(count);  // Best value for #QPU=4
}

/**
 * @brief Interface to use VPM as shared memory
 *
 * VPM read/write is slower than you would expect, because it is 
 * not a direct memory access.  
 * VPM reads/writes are both put in a request queue and are handled
 * sequentially.  
 * The perfomance is honestly disappointing, but is offset by the utility
 * of having shared memory over the QPU's.
 */
class VPMArray {
public:
  void set(IntExpr const &addr, IntExpr const &val);
  IntExpr get(IntExpr const &addr);

private:
  bool m_wait = false;
};

VPMArray vpm;

void VPMArray::set(IntExpr const &addr, IntExpr const &val) {
  vpmSetupWrite(HORIZ, addr);
  vpmPut(val);

  m_wait = true;
}


IntExpr VPMArray::get(IntExpr const &addr) {
  if (m_wait) {
    //
    // The implementation implies that the current QPU is waiting for its own
    // write to complete.
    //
    // This is NOT the case; in actual case the QPU is waiting for any OTHER
    // QPU to complete their write.
    //
    // The assumption here is that all QPU's do the same write, so that the execution
    // is comparable.
    //
    add_nop();
    m_wait = false;
  }

  vpmSetupRead(HORIZ, 1, addr);
  return vpmGetInt();
}


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

  add_nop();  // TODO: Check if calling this just before vpmGetInt() makes a difference

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
