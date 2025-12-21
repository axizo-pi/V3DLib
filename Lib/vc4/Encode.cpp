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
      instr.ALU.op  = ALUOp(ALUOp::A_BOR);
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


/**
 * Handle case where there are two source operands
 *
 * This is fairly convoluted stuff; apparently there are rules with regfile A/B usage
 * which I am not aware of.
 */
void encode_operands(Instr &instr, RegOrImm const &srcA, RegOrImm const &srcB) {
  uint8_t muxa, muxb;
  uint8_t raddr_a = 0, raddr_b = 0;

  if (srcA.is_reg() && srcB.is_reg()) { // Both operands are registers
    RegTag aFile = srcA.reg().regfile();
    RegTag aTag  = srcA.reg().tag;

    RegTag bFile = srcB.reg().regfile();

    // If operands are the same register
    if (aTag != NONE && srcA == srcB) {
      if (aFile == REG_A) {
        raddr_a = vc4::Instr::encodeSrcReg(srcA.reg(), REG_A, muxa);
        muxb = muxa;
      } else {
        raddr_b = vc4::Instr::encodeSrcReg(srcA.reg(), REG_B, muxa);
        muxb = muxa;
      }
    } else {
      // Operands are different registers
      assert(aFile == NONE || bFile == NONE || aFile != bFile);  // TODO examine why aFile == bFile is disallowed here
      if (aFile == REG_A || bFile == REG_B) {
        raddr_a = vc4::Instr::encodeSrcReg(srcA.reg(), REG_A, muxa);
        raddr_b = vc4::Instr::encodeSrcReg(srcB.reg(), REG_B, muxb);
      } else {
        raddr_a = vc4::Instr::encodeSrcReg(srcB.reg(), REG_A, muxb);
        raddr_b = vc4::Instr::encodeSrcReg(srcA.reg(), REG_B, muxa);
      }
    }
  } else if (srcA.is_imm() || srcB.is_imm()) {
    if (srcA.is_imm() && srcB.is_imm()) {
      assertq(srcA.imm() == srcB.imm(),
        "srcA and srcB can not both be immediates with different values", true);

      raddr_b = srcA.encode();  // srcB is the same
      muxa   = Instr::MUX_B;
      muxb   = Instr::MUX_B;
    } else if (srcB.is_imm()) {
      // Second operand is a small immediate
      raddr_a = vc4::Instr::encodeSrcReg(srcA.reg(), REG_A, muxa);
      raddr_b = srcB.encode();
      muxb   = Instr::MUX_B;
    } else if (srcA.is_imm()) {
      // First operand is a small immediate
      raddr_a = vc4::Instr::encodeSrcReg(srcB.reg(), REG_A, muxb);
      raddr_b = srcA.encode();
      muxa   = Instr::MUX_B;
    } else {
      assert(false);  // Not expecting this
    }
  } else {
    assert(false);  // Not expecting this
  }

  instr.raddr_a  = raddr_a;
  instr.raddr_b  = raddr_b;
  instr.add_a    = muxa;
  instr.add_b    = muxb;
}


// ===================
// Instruction encoder
// ===================

uint64_t encodeInstr(Target::Instr instr) {
  convertInstr(instr);

  vc4::Instr vc4_instr;

  // Encode core instruction
  switch (instr.tag) {
    case NO_OP:
      break; // Use default value for instr, which is a full NOP

    case LI: {        // Load immediate
      auto &li = instr.LI;
      RegTag file;

      vc4_instr.enc       = vc4::Instr::LOAD_IMM;
      vc4_instr.cond_add  = instr.assign_cond().encode();
      vc4_instr.waddr_add = vc4::Instr::encodeDestReg(instr.dest(), &file);
      vc4_instr.ws        = (file != REG_A);
      vc4_instr.immediate = li.imm.encode();
      vc4_instr.sf        = instr.set_cond().flags_set();
    }
    break;

    case BR:  // Branch
      assertq(!instr.branch_target().useRegOffset, "Register offset not yet supported");

      vc4_instr.enc       = vc4::Instr::BRANCH_ENC;
      vc4_instr.cond_add  = instr.branch_cond().encode();
      vc4_instr.rel       = instr.branch_target().relative;
      vc4_instr.immediate = 8*instr.branch_target().immOffset;
    break;

    case ALU: {
      auto &alu = instr.ALU;

      RegTag file;
      uint8_t dest = vc4::Instr::encodeDestReg(instr.dest(), &file);

      if (alu.op.isMul()) {
        vc4_instr.cond_mul  = instr.assign_cond().encode();
        vc4_instr.waddr_mul = dest;
        vc4_instr.ws        = (file != REG_B);
      } else {
        vc4_instr.cond_add  = instr.assign_cond().encode();
        vc4_instr.waddr_add = dest;
        vc4_instr.ws        = (file != REG_A);
      }

      vc4_instr.sf = (instr.set_cond().flags_set());

      if (alu.op.isRot()) {
        assert(alu.srcA.is_reg() && alu.srcA.reg().tag == ACC && alu.srcA.reg().regId == 0);
        assert(!alu.srcB.is_reg() || (alu.srcB.reg().tag == ACC && alu.srcB.reg().regId == 5));
        uint8_t raddr_b = 48;

        if (!alu.srcB.is_reg()) {  // i.e. value is an imm

// Grumbl mofo compiler insists that n is int
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
          uint8_t n = alu.srcB.encode();
          assert(n >= 1 || n <= 15);
          raddr_b += n;
#pragma GCC diagnostic pop

        }

        vc4_instr.enc     = Instr::ALU_SMALL_IMM;
        vc4_instr.op_mul  = ALUOp(ALUOp::M_V8MIN).vc4_encodeMulOp();
        vc4_instr.raddr_b = raddr_b;
      } else {
        if (instr.hasImm()) {
          vc4_instr.enc  = Instr::ALU_SMALL_IMM;
        } else {
          vc4_instr.enc  = Instr::ALU;
        }
        vc4_instr.op_mul = (alu.op.isMul() ? alu.op.vc4_encodeMulOp() : 0);
        vc4_instr.op_add = (alu.op.isMul() ? 0 : alu.op.vc4_encodeAddOp());
        encode_operands(vc4_instr, alu.srcA, alu.srcB);
      }
    }
    break;

    // Halt
    case END:
      //vc4_instr.tag(vc4::Instr::END);
      vc4_instr.enc     = Instr::ALU;
      vc4_instr.sig     = Instr::PROGRAM_END;
      vc4_instr.raddr_b = Instr::NOP_R;
      break;

    case RECV: {
      assert(instr.dest() == Reg(ACC,4));  // ACC4 is the only value allowed as dest
      //vc4_instr.tag(vc4::Instr::LDTMU);
      vc4_instr.enc     = Instr::ALU;
      vc4_instr.sig     = Instr::LOAD_FROM_TMU0;
      vc4_instr.raddr_b = Instr::NOP_R;
    }
    break;

    // Semaphore increment/decrement
    case SINC:
      //vc4_instr.tag(vc4::Instr::SINC);
      //vc4_instr.sema_id = instr.semaId;
      vc4_instr.enc       = Instr::SEMAPHORE;
      vc4_instr.semaphore = instr.semaId;
      vc4_instr.sa        = false;
      break;

    case SDEC:
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
