#ifndef _LIB_V3D_COMBINE_H
#define _LIB_V3D_COMBINE_H
#include "instr/Instr.h"

namespace V3DLib {
namespace v3d {
namespace Combine {

using V3DLib::v3d::instr::Instr;

int optimize(Instructions &instrs);

}  // namespace Combine
}  // namespace v3d
}  // namespace V3DLib


#endif //  _LIB_V3D_COMBINE_H
