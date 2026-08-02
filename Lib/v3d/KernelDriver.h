#ifndef _V3DLIB_V3D_KERNELDRIVER_H
#define _V3DLIB_V3D_KERNELDRIVER_H
#include "Compile.h"
#include "../KernelDriver.h"
#include "Common/SharedArray.h"
#include "BufferObject.h"
#include "Driver.h"

namespace V3DLib {
namespace v3d {

/**
 * @brief Buffer Object for v3d
 *
 * The generated code is loaded into a separate BO.
 * This is to be able to load and run multiple kernels for v3d in the same context.
 *
 * Loading multiple kernels into the same Buffer Object (BO) didn't work for unfathomable reasons,
 * and resulted in run timeouts and eventually locked up the pi4.
 * vc4 does not have this issue.
 */
class KernelDriver : public V3DLib::KernelDriver {
  using Parent       = V3DLib::KernelDriver;
  using Instruction  = V3DLib::v3d::instr::Instr;
  using Instructions = V3DLib::v3d::Instructions;

public:
  KernelDriver() {}
  KernelDriver(KernelDriver &&a) = default;

  void invoke(V3DLib::Compile &code, int numQPUs, IntList &params, bool wait_complete = true) override;
  void wait_complete() override;

private:
  Data   uniforms;
  Data   devnull;
  Data   done;
  Driver drv;
};

}  // namespace v3d
}  // namespace V3DLib

#endif  // _V3DLIB_V3d_KERNELDRIVER_H
