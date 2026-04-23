#ifndef _V3DLIB_SOURCE_TRANSLATE_H_
#define _V3DLIB_SOURCE_TRANSLATE_H_
#include "Source/Stmt.h"
#include "Target/instr/Instr.h"

namespace V3DLib {

void encode_source(Instr::List &target, Stmt::Array const &source);
void insert_init_block(Instr::List &code, Instr::List &init);

//
// Following exposed for DMA.
//
Instr::List varAssign(Var v, Expr::Ptr expr);

}  // namespace V3DLib

#endif  // _V3DLIB_SOURCE_TRANSLATE_H_
