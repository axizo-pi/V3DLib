#ifndef _V3DLIB_EMULATOR_EMULATOR_H_
#define _V3DLIB_EMULATOR_EMULATOR_H_
#include "Target/instr/Instr.h"

namespace V3DLib {

class BufferObject;

void emulate(int numQPUs, Target::Instr::List &instrs, int maxReg, IntList &uniforms, BufferObject &heap);

}  // namespace V3DLib

#endif  // _V3DLIB_EMULATOR_EMULATOR_H_
