#ifndef _V3DLIB_SATISFY_H_
#define _V3DLIB_SATISFY_H_
#include "Target/instr/Instr.h"

namespace V3DLib {

void adjust_immediates(Target::Instr::List &instrs);
void vc4_satisfy(Target::Instr::List &instrs);
void v3d_satisfy(Target::Instr::List &instrs);

}  // namespace V3DLib

#endif  // _V3DLIB_SATISFY_H_
