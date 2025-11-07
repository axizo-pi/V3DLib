#include "Location.h"
#include "global/log.h"
#include "Support/Platform.h"

using namespace Log;

namespace V3DLib {
namespace v3d {
namespace instr {

/**
 * General purpose acc's not present on vc7; block usage in this case.
 */
bool Location::check_acc_usage(Location const &loc) {
	if (!V3DLib::Platform::compiling_for_vc7()) return true;

	if (loc.is_rf()) return true;

	debug << "Checking Location with mux '" << loc.to_mux() << "'";

	auto mux = loc.to_mux();

	if (
   mux == V3D_QPU_MUX_R0 ||
   mux == V3D_QPU_MUX_R1 ||
   mux == V3D_QPU_MUX_R2 ||
   mux == V3D_QPU_MUX_R3 ||
   mux == V3D_QPU_MUX_R4
	) {
		fatal << "Can not use general purpose registers on vc7";
		return false;
	}

	return true;
}	

}  // instr
}  // v3d
}  // V3DLib
