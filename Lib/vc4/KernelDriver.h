#ifndef _LIB_VC4_KERNELDRIVER_H
#define _LIB_VC4_KERNELDRIVER_H
#include "../KernelDriver.h"
#include "Common/SharedArray.h"
#include "Encode.h"
#include "Invoke.h"

namespace V3DLib {
namespace vc4 {

class KernelDriver : public V3DLib::KernelDriver, private MailBoxInvoke {
  using Parent = V3DLib::KernelDriver;

public:
  KernelDriver();
  KernelDriver(KernelDriver &&k) = default;

  void invoke(int numQPUs, IntList &params, bool wait_complete = true) override;
  void encode() override;
  int kernel_size() const override;

private:
  void compile_intern() override;
	std::string emit_opcodes() override;
};

}  // namespace vc4
}  // namespace V3DLib

#endif  // _LIB_VC4_KERNELDRIVER_H
