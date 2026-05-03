#ifndef _V3DLIB_V3D_COMBINE_H
#define _V3DLIB_V3D_COMBINE_H
#include "instr/Instr.h"

namespace V3DLib {
namespace v3d {
namespace Combine {

using V3DLib::v3d::instr::Instr;

int optimize(Instructions &instrs);

}  // namespace Combine
}  // namespace v3d
}  // namespace V3DLib

#endif //  _V3DLIB_V3D_COMBINE_H
