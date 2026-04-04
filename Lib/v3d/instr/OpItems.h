#ifndef _V3DLIB_V3D_INSTR_OPITEMS_H
#define _V3DLIB_V3D_INSTR_OPITEMS_H
#include "Target/instr/Instr.h"

namespace V3DLib {
namespace v3d {
namespace instr {

class OpItems {
public:
  static bool get_add_op(ALUInstruction const &add_alu, v3d_qpu_add_op &dst, bool strict = false);
  static bool get_mul_op(ALUInstruction const &add_alu, v3d_qpu_mul_op &dst);

private:
  static bool uses_add_alu(Target::Instr const &instr);
  static bool uses_mul_alu(Target::Instr const &instr);
};


}  // namespace instr
}  // namespace v3d
}  // namespace V3DLib

#endif  // _V3DLIB_V3D_INSTR_OPITEMS_H
