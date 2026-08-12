#ifndef _V3DLIB_V3D_COMPILE_H
#define _V3DLIB_V3D_COMPILE_H
#include "../Compile.h"
#include "KernelDriver.h"
#include "UniformConstants.h"
#include "instr/Instr.h"

namespace V3DLib {
namespace v3d {

class Compile: public V3DLib::Compile {
  using Instruction  = V3DLib::v3d::instr::Instr;
  using Instructions = V3DLib::v3d::Instructions;

public:  
  Compile();

  int kernel_size() const override { return (int) instructions.size(); }
  void encode() override;
  void allocate() override;

  void invoke(int numQPUs, IntList &params, bool wait_complete = true) override;
  void wait_complete() override { return m_driver.wait_complete(); }

private:  
  Instructions      instructions;
  UniformConstants  m_uc;
	v3d::KernelDriver m_driver;

  void compile_intern() override;
  std::string emit_opcodes() override;
  void init_uniforms() override;

  ByteCode to_opcodes();
};

} // namespace v3d
} // namespace V3DLib

#endif // _V3DLIB_V3D_COMPILE_H

