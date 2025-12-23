#include "Location.h"
//#include "global/log.h"
#include "Support/Platform.h"
#include "Support/basics.h"

using namespace Log;
//using ::operator<<; // C++ weirdness

namespace V3DLib {
namespace v3d {
namespace instr {


/**
 * General purpose acc's not present on vc7; block usage in this case.
 */
bool Location::check_acc_usage(Location const &loc) {
	if (V3DLib::Platform::compiling_for_vc7()) return true;

	if (loc.is_rf()) return true;

	Log::debug << "Checking Location with mux '" << loc.to_mux() << "'";

	auto mux = loc.to_mux();

	if (
   mux == V3D_QPU_MUX_R0 ||
   mux == V3D_QPU_MUX_R1 ||
   mux == V3D_QPU_MUX_R2 ||
   mux == V3D_QPU_MUX_R3 ||
   mux == V3D_QPU_MUX_R4
	) {
		return false;
	}

	return true;
}	


std::string Location::dump() const {
	std::string ret;

  if (is_rf()) {
		ret << "rf";
	} else if (is_acc()) {
		ret << "r";
	} else {
		ret << "special ";
	}

  ret << to_waddr();
	return ret;
}

}  // instr
}  // v3d
}  // V3DLib
