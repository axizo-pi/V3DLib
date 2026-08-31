#ifndef _V3DLIB_EMULATOR_EMULATOR_H_
#define _V3DLIB_EMULATOR_EMULATOR_H_
#include "Common/Seq.h"  // IntList

namespace V3DLib {

class BufferObject;
class CodeStruct;

void emulate(
  int numQPUs,
  CodeStruct const &cs,
  int maxReg,
  IntList &uniforms,
  BufferObject &heap,
  bool do_debug
);

}  // namespace V3DLib

#endif  // _V3DLIB_EMULATOR_EMULATOR_H_
