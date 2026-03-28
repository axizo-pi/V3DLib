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
 * VPM is vc4-only.
 */
void vpm_kernel(Int::Ptr ret) {
	Int tmp = 10*(me() + 1);

	// Write own value to VPM
  vpmSetupWrite(HORIZ, me());
  vpmPut(tmp);

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
  LibSettings::tmu_load tmu(false);
	int numQPUs = 12;

	auto init_expected = [numQPUs] (Int::Array &expected) {
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
		init_expected(expected);

	  auto k = compile(vpm_kernel);
	  k.setNumQPUs(numQPUs);
	  k.load(&result);
	  k.emu();

		REQUIRE(result == expected);
	}


  SUBCASE("on QPUs") {
		if (!Platform::compiling_for_vc4()) {
			warn << "Not running VPM mem on QPU's for v3d";
			return;
		}

	  Int::Array result(numQPUs*16);

    Int::Array expected(numQPUs*16);
		init_expected(expected);

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
