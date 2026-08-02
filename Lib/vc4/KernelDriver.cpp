#include "KernelDriver.h"
#include "RegisterMap.h"      // ErrorStatus()()

namespace V3DLib {

using ::operator<<;  // C++ weirdness
using namespace std;

namespace vc4 {

void KernelDriver::invoke(V3DLib::Compile &code, int numQPUs, IntList &params, bool wait_complete) {
  assert(params.size() != 0);

  if (code.has_errors()) {
    fatal("Errors during kernel compilation/encoding, can't continue.");
  }

  if (!wait_complete) {
    warn << "run(): disabling wait completion only works for v3d. Ignoring for vc4.";
  }

  try {
    MailBoxInvoke::invoke(numQPUs, code.code(), params);
  } catch (std::runtime_error const &e) {
    Log::cerr << "KernelDriver::invoke exception caught: " << e.what() << "\n"
              << "Error registers:\n"
              << RegisterMap::ErrorStatus();
    throw;
  }
}

}  // namespace vc4
}  // namespace V3DLib

