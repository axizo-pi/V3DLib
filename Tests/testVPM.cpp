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
  Platform::main_mem mem(true);

  int numQPUs = 4;
  Int::Array result(numQPUs*16);

  auto k = compile(vpm_kernel);
  k.setNumQPUs(numQPUs);
  to_file("vpm_kernel.txt", k.dump());
  k.load(&result);
  k.emu();

	std::cout << result.dump() << "\n";
}
