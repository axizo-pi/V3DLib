#ifndef _V3DLIB_EMULATOR_INTERPRETER_H_
#define _V3DLIB_EMULATOR_INTERPRETER_H_
#include "Common/Seq.h"           // IntList

namespace V3DLib {

class BufferObject;
class CodeStruct;

template<typename T>
class Seq;

void interpreter(
  int numCores,
	CodeStruct const &cs,
  int numVars,
  IntList &uniforms,
  BufferObject &heap
);

}  // namespace V3DLib

#endif  // _V3DLIB_INTERPRETER_H_
