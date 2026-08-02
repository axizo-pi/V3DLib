#ifndef _V3DLIB_V3D_COMPILE_H
#define _V3DLIB_V3D_COMPILE_H
#include "../Compile.h"
#include "instr/Instr.h"

namespace V3DLib {
namespace v3d {

class Compile: public V3DLib::Compile {
public:  
  Compile();

  int kernel_size() const override { return (int) instructions.size(); }
  void encode() override;
  void allocate() override;

private:  
  Instructions  instructions;

  void compile_intern() override;
  std::string emit_opcodes() override;
  void init_uniforms() override;

  ByteCode to_opcodes();
};

} // namespace v3d
} // namespace V3DLib

#endif // _V3DLIB_V3D_COMPILE_H

