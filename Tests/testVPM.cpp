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
 *     - 80: all filled except #3
 *     - 70: all filled except #3
 * * zero
 *   + 4 QPU's, count:
 *     - 90: all filled
 *     - 80 #2 not filled, rest filled
 *     - 70 #2 not filled, rest filled
 */
void add_nop() {
  if (Platform::use_main_memory()) {
    // Must be compiling for emulator, don't bother
    return;
  }

  int count = 80;

  if (Platform::tag() == Platform::pi_zero) {
    count = 90;
  }

  nop(count);  // Best value for #QPU=4
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
  Int tmp2 = (me() + 1);  comment("Read value from other QPU");
	Where (tmp2 == numQPUs())
		tmp2 = 0;
	End

  vpmSetupRead(HORIZ, 1, tmp2);   
	Int tmp3 = vpmGetInt();

	*(ret + 16*me()) = tmp3;
}

}

/**
 * Test using VPM as shared memory for the QPU's.
 *
 * Note that VPM as mem and DMA will interfere in the VPM usage!
 * Just don't do DMA when using VPM as mem.
 */
TEST_CASE("Test VPM memory [vpm]") {
  warn << "tag: " << Platform::tag();

  LibSettings::tmu_load tmu(false);
	int numQPUs = 4;

	auto init_expected = [] (Int::Array &expected, int numQPUs) {
		for (int i = 0; i < numQPUs; ++i) {
			for (int j = 0; j < 16; ++j) {
				expected[16*((i + (numQPUs - 1)) % numQPUs) + j] = 10*(i + 1);
			}
		}
	};

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

		std::cout << result.dump() << "\n";
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
	  to_file("vpm_kernel.txt", k.dump());
	  k.setNumQPUs(numQPUs);
	  k.load(&result);
	  k.run();

		std::cout << result.dump() << "\n";
		//std::cout << expected.dump() << "\n";
		REQUIRE(result == expected);
	}
}
