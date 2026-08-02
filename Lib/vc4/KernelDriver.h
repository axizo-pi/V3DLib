#ifndef _V3DLIB_VC4_KERNELDRIVER_H
#define _V3DLIB_VC4_KERNELDRIVER_H
#include "../KernelDriver.h"
#include "Invoke.h"
#include "Compile.h"

namespace V3DLib {
namespace vc4 {

class KernelDriver : public V3DLib::KernelDriver, private MailBoxInvoke {
  using Parent = V3DLib::KernelDriver;

public:
  KernelDriver() {}
  KernelDriver(KernelDriver &&k) = default;

  void invoke(V3DLib::Compile &code, int numQPUs, IntList &params, bool wait_complete = true) override;
};

}  // namespace vc4
}  // namespace V3DLib

#endif  // _V3DLIB_VC4_KERNELDRIVER_H
