#ifndef _LIB_V3D_COMBINE_H
#define _LIB_V3D_COMBINE_H
#include "instr/Instr.h"

namespace V3DLib {
namespace v3d {
namespace Combine {

using V3DLib::v3d::instr::Instr;

void remove_skips(Instructions &instr);

bool add_alu_to_mul_alu(Instr const &in_instr, Instr &dst);
void combine(Instructions &instr);

}  // namespace Combine
}  // namespace v3d
}  // namespace V3DLib


#endif //  _LIB_V3D_COMBINE_H
