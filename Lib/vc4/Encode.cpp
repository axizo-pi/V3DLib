#include "Encode.h"
#include <stdio.h>
#include <stdlib.h>
#include "Support/basics.h"  // fatal()
#include "Target/Satisfy.h"
#include "Instr.h"

namespace V3DLib {

using namespace Target;

namespace vc4 {
namespace {

/**
 * Convert intermediate instruction into core instruction
 */
void convertInstr(Target::Instr &instr) {
  switch (instr.tag) {
    case IRQ:
      instr.tag           = LI;
      instr.LI.imm        = Imm(1);
      instr.set_cond_clear();
      instr.assign_cond(AssignCond(AssignCond::Tag::ALWAYS));
      instr.dest(Reg(SPECIAL, SPECIAL_HOST_INT));
      break;

    case DMA_LOAD_WAIT:
    case DMA_STORE_WAIT: {
      RegId src = instr.tag == DMA_LOAD_WAIT ? SPECIAL_DMA_LD_WAIT : SPECIAL_DMA_ST_WAIT;

      instr.tag     = ALU;
      instr.ALU.op  = ALUOp(Enum::A_BOR);
      instr.set_cond_clear();
      instr.assign_cond(AssignCond(AssignCond::Tag::NEVER));

      instr.ALU.srcA = Reg(SPECIAL, src);  // srcA is same as srcB
      instr.ALU.srcB = Reg(SPECIAL, src);
      instr.dest(Reg(NONE, 0));
      break;
    }

    default:
      break;  // rest passes through
  }
}


// ===================
// Instruction encoder
// ===================

uint64_t encodeInstr(Target::Instr &instr) {
  convertInstr(instr);

  vc4::Instr vc4_instr;
  vc4_instr.encode(instr); 

  return vc4_instr.encode();
}

}  // anon namespace


// ============================================================================
// Top-level encoder
// ============================================================================

CodeList encode(Target::Instr::List &instrs) {
  CodeList code;

  for (int i = 0; i < instrs.size(); i++) {
    auto instr = instrs.get(i);
    check_instruction_tag_for_platform(instr.tag, true);

    if (instr.tag == INIT_BEGIN || instr.tag == INIT_END) {
      continue;  // Don't encode these block markers
    }

    code << encodeInstr(instr);
  }

  return code;
}

}  // namespace vc4
}  // namespace V3DLib
