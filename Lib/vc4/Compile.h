#ifndef _V3DLIB_VC4_COMPILE_H
#define _V3DLIB_VC4_COMPILE_H
#include "../Compile.h"
#include "KernelDriver.h"

namespace V3DLib {
namespace vc4 {

class Compile: public V3DLib::Compile {
public:  
  Compile();

  int kernel_size() const override;
  void encode() override;

  void invoke(int numQPUs, IntList &params, bool wait_complete = true) override;

private:
  V3DLib::vc4::KernelDriver m_driver;

  void compile_intern() override;
  std::string emit_opcodes() override;
};

} // namespace V3DLib
} // namespace vc4

#endif // _V3DLIB_VC4_COMPILE_H

