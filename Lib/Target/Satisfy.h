#ifndef _V3DLIB_SATISFY_H_
#define _V3DLIB_SATISFY_H_
#include "Target/instr/Instr.h"

namespace V3DLib {

Instr::List adjust_immediates(Instr::List const &instrs);
void vc4_satisfy(Instr::List &instrs);
void v3d_satisfy(Instr::List &instrs);

}  // namespace V3DLib

#endif  // _V3DLIB_SATISFY_H_
