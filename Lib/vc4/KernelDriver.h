#ifndef _V3DLIB_VC4_KERNELDRIVER_H
#define _V3DLIB_VC4_KERNELDRIVER_H
#include "Invoke.h"

namespace V3DLib {
namespace vc4 {

class KernelDriver : private MailBoxInvoke {
public:
  KernelDriver() {}
  KernelDriver(KernelDriver &&k) = default;

  void invoke(V3DLib::Code const &code, int numQPUs, IntList &params, bool wait_complete = true);
};

}  // namespace vc4
}  // namespace V3DLib

#endif  // _V3DLIB_VC4_KERNELDRIVER_H
