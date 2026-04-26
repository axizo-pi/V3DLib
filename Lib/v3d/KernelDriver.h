#ifndef _LIB_V3D_KERNELDRIVER_H
#define _LIB_V3D_KERNELDRIVER_H
#ifdef QPU_MODE
#include "../KernelDriver.h"
#include "Common/SharedArray.h"
#include "instr/Instr.h"
#include "BufferObject.h"
#include "Driver.h"
#include "UniformConstants.h"

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
  KernelDriver();
  KernelDriver(KernelDriver &&a) = default;

  void invoke(int numQPUs, IntList &params, bool wait_complete = true) override;
  void encode() override;
  int kernel_size() const override { return (int) instructions.size(); }
  void wait_complete() override;

private:
  Instructions  instructions;

  Data   uniforms;
  Data   devnull;
  Data   done;
  Driver drv;
	UniformConstants m_uniform_constants;

  void compile_intern() override;

  void allocate();
  ByteCode to_opcodes();
  std::string emit_opcodes() override;
	void init_uniforms() override;
};

}  // namespace v3d
}  // namespace V3DLib

#endif  // QPU_MODE
#endif  // _LIB_V3d_KERNELDRIVER_H
