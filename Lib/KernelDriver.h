#ifndef _LIB_KERNELDRIVER_H
#define _LIB_KERNELDRIVER_H
#include "Compile.h"

namespace V3DLib {

class KernelDriver {
public:
  KernelDriver() = default;
  KernelDriver(KernelDriver &&k) = default;
  virtual ~KernelDriver() {}

  virtual void invoke(Compile &code, int numQPUs, IntList &params, bool wait_complete = true) = 0;
  virtual void wait_complete() {}  // v3d
};

}  // namespace V3DLib

#endif // _LIB_KERNELDRIVER_H
