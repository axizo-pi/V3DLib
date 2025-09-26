#include "ALUOp.h"
#include <stdint.h>
#include "Support/basics.h"
#include "Source/Op.h"

namespace V3DLib {

ALUOp::ALUOp(Op const &op) : m_value(op.opcode()) {}


/**
 * Determine if current is a mul-ALU instruction
    bool uncond = instr.tag == BRL && instr.branch_cond().tag == COND_ALWAYS;
 * Determines if the mul-ALU needs to be used
 *
 * TODO: Examine if this is still true for v3d
 */
bool ALUOp::isMul() const {
  return (M_FMUL <= m_value && m_value <= M_ROTATE);
}


std::string ALUOp::pretty() const {
  switch (m_value) {
    case NOP:       return "nop";
    case A_FADD:    return "addf";
    case A_FSUB:    return "subf";
    case A_FMIN:    return "minf";
    case A_FMAX:    return "maxf";
    case A_FMINABS: return "minabsf";
    case A_FMAXABS: return "maxabsf";
    case A_FtoI:    return "ftoi";
    case A_ItoF:    return "itof";
    case A_ADD:     return "add";
    case A_SUB:     return "sub";
    case A_SHR:     return "shr";
    case A_ASR:     return "asr";
    case A_ROR:     return "ror";
    case A_SHL:     return "shl";
    case A_MIN:     return "min";
    case A_MAX:     return "max";
    case A_BAND:    return "and";
    case A_BOR:     return "or";
    case A_BXOR:    return "xor";
    case A_BNOT:    return "not";
    case A_CLZ:     return "clz";
    case A_V8ADDS:  return "addsatb";
    case A_V8SUBS:  return "subsatb";
    case M_FMUL:    return "fmul";
    case M_MUL24:   return "mul24";
    case M_V8MUL:   return "mulb";
    case M_V8MIN:   return "minb";
    case M_V8MAX:   return "maxb";
    case M_V8ADDS:  return "m_addsatb";
    case M_V8SUBS:  return "m_subsatb";
    case M_ROTATE:  return "rotate";
    // v3d-specific
    case A_TIDX:    return "tidx";
    case A_EIDX:    return "eidx";
    case A_FFLOOR:  return "ffloor";
    case A_FSIN:    return "sin";
    case A_TMUWT:   return "tmuwt";
    default:
      assertq(false, "pretty(): Unknown ALU opcode", true);
      return "";
  }
}


uint32_t ALUOp::vc4_encodeAddOp() const {

/////////////////////////////////////////////////////////
// Here is the reason vc4 wasn't working!!!!
/////////////////////////////////////////////////////////

/* <<<<<<<<<<<<<<<<<<<<<<<<
  if (NOP <= m_value && m_value <= A_V8SUBS) return m_value;
   ======================== */
  switch (m_value) {
    case NOP:       return 0;
    case A_FADD:    return 1;
    case A_FSUB:    return 2;
    case A_FMIN:    return 3;
    case A_FMAX:    return 4;
    case A_FMINABS: return 5;
    case A_FMAXABS: return 6;
    case A_FtoI:    return 7;
    case A_ItoF:    return 8;
    case A_ADD:     return 12;
    case A_SUB:     return 13;
    case A_SHR:     return 14;
    case A_ASR:     return 15;
    case A_ROR:     return 16;
    case A_SHL:     return 17;
    case A_MIN:     return 18;
    case A_MAX:     return 19;
    case A_BAND:    return 20;
    case A_BOR:     return 21;
    case A_BXOR:    return 22;
    case A_BNOT:    return 23;
    case A_CLZ:     return 24;
    case A_V8ADDS:  return 30;
    case A_V8SUBS:  return 31;
/* >>>>>>>>>>>>>>>>>>>>>>>> */

/* <<<<<<<<<<<<<<<<<<<<<<<<
  assertq("V3DLib: unknown add op", true);
  return 0;
   ======================== */
    default:
      assertq("V3DLib: unknown add op", true);
      return 0;
  }
/* >>>>>>>>>>>>>>>>>>>>>>>> */
}


uint32_t ALUOp::vc4_encodeMulOp() const {

/////////////////////////////////////////////////////////
// Here is the second reason vc4 wasn't working!!!!
/////////////////////////////////////////////////////////

/* <<<<<<<<<<<<<<<<<<<<<<<<
  if (m_value == NOP) return NOP;
  if (isMul() && m_value != M_ROTATE) return m_value - M_FMUL; 
   ======================== */
  switch (m_value) {
    case NOP:      return 0;
    case M_FMUL:   return 1;
    case M_MUL24:  return 2;
    case M_V8MUL:  return 3;
    case M_V8MIN:  return 4;
    case M_V8MAX:  return 5;
    case M_V8ADDS: return 6;
    case M_V8SUBS: return 7;
/* >>>>>>>>>>>>>>>>>>>>>>>> */

/* <<<<<<<<<<<<<<<<<<<<<<<<
  fatal("V3DLib: unknown MUL op");
  return 0;
   ======================== */
    default:
      fatal("V3DLib: unknown mul op");
      return 0;
  }
/* >>>>>>>>>>>>>>>>>>>>>>>> */
}

}  // namespace V3DLib
