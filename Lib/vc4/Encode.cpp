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

  // Encode core instruction
  switch (instr.tag) {
    case NO_OP:
      breakpoint;
      break; // Use default value for instr, which is a full NOP

    case BR:  // Branch
      breakpoint;
      assertq(!instr.branch_target().useRegOffset, "Register offset not yet supported");

      vc4_instr.enc       = vc4::Instr::BRANCH_ENC;
      vc4_instr.cond_add  = instr.branch_cond().encode();
      vc4_instr.rel       = instr.branch_target().relative;
      vc4_instr.immediate = 8*instr.branch_target().immOffset;
    break;

    case LI:        // Load immediate
    case ALU:
      vc4_instr.encode(instr); 
    break;

    // Halt
    case END:
      breakpoint;
      //vc4_instr.tag(vc4::Instr::END);
      vc4_instr.enc     = Instr::ALU;
      vc4_instr.sig     = Instr::PROGRAM_END;
      vc4_instr.raddr_b = Instr::NOP_R;
      break;

    case RECV: {
      breakpoint;
      assert(instr.dest() == Reg(ACC,4));  // ACC4 is the only value allowed as dest
      //vc4_instr.tag(vc4::Instr::LDTMU);
      vc4_instr.enc     = Instr::ALU;
      vc4_instr.sig     = Instr::LOAD_FROM_TMU0;
      vc4_instr.raddr_b = Instr::NOP_R;
    }
    break;

    // Semaphore increment/decrement
    case SINC:
      breakpoint;
      //vc4_instr.tag(vc4::Instr::SINC);
      //vc4_instr.sema_id = instr.semaId;
      vc4_instr.enc       = Instr::SEMAPHORE;
      vc4_instr.semaphore = instr.semaId;
      vc4_instr.sa        = false;
      break;

    case SDEC:
      breakpoint;
      //vc4_instr.tag(vc4::Instr::SDEC);
      //vc4_instr.sema_id = instr.semaId;
      vc4_instr.enc       = Instr::SEMAPHORE;
      vc4_instr.semaphore = instr.semaId;
      vc4_instr.sa        = true;
      break;

    default:
      assertq(false, "encodeInstr(): Target lang instruction tag not handled");
      break;
  }

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
