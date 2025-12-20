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
void encode_operands(Instr &vc4_instr, RegOrImm const &srcA, RegOrImm const &srcB) {
  uint32_t muxa, muxb;
  uint32_t raddra = 0, raddrb;

  if (srcA.is_reg() && srcB.is_reg()) { // Both operands are registers
    RegTag aFile = srcA.reg().regfile();
    RegTag aTag  = srcA.reg().tag;

    RegTag bFile = srcB.reg().regfile();

    // If operands are the same register
    if (aTag != NONE && srcA == srcB) {
      if (aFile == REG_A) {
        raddra = vc4::Instr::encodeSrcReg(srcA.reg(), REG_A, &muxa);
        raddrb = Instr::NOP_R;
        muxb = muxa;
      } else {
        raddra = Instr::NOP_R;
        raddrb = vc4::Instr::encodeSrcReg(srcA.reg(), REG_B, &muxa);
        muxb = muxa;
      }
    } else {
      // Operands are different registers
      assert(aFile == NONE || bFile == NONE || aFile != bFile);  // TODO examine why aFile == bFile is disallowed here
      if (aFile == REG_A || bFile == REG_B) {
        raddra = vc4::Instr::encodeSrcReg(srcA.reg(), REG_A, &muxa);
        raddrb = vc4::Instr::encodeSrcReg(srcB.reg(), REG_B, &muxb);
      } else {
        raddra = vc4::Instr::encodeSrcReg(srcB.reg(), REG_A, &muxb);
        raddrb = vc4::Instr::encodeSrcReg(srcA.reg(), REG_B, &muxa);
      }
    }
  } else if (srcA.is_imm() || srcB.is_imm()) {
    if (srcA.is_imm() && srcB.is_imm()) {
      assertq(srcA.imm() == srcB.imm(),
        "srcA and srcB can not both be immediates with different values", true);

      raddrb = srcA.encode();  // srcB is the same
      muxa   = Instr::MUX_B;
      muxb   = Instr::MUX_B;
    } else if (srcB.is_imm()) {
      // Second operand is a small immediate
      raddra = vc4::Instr::encodeSrcReg(srcA.reg(), REG_A, &muxa);
      raddrb = srcB.encode();
      muxb   = Instr::MUX_B;
    } else if (srcA.is_imm()) {
      // First operand is a small immediate
      raddra = vc4::Instr::encodeSrcReg(srcB.reg(), REG_A, &muxb);
      raddrb = srcA.encode();
      muxa   = Instr::MUX_B;
    } else {
      assert(false);  // Not expecting this
    }
  } else {
    assert(false);  // Not expecting this
  }

  vc4_instr.raddra  = raddra;
  vc4_instr.raddrb  = raddrb;
  vc4_instr.muxa    = muxa;
  vc4_instr.muxb    = muxb;
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

      vc4_instr.tag(vc4::Instr::LI);
      vc4_instr.cond_add  = instr.assign_cond().encode();
      vc4_instr.waddr_add = vc4::Instr::encodeDestReg(instr.dest(), &file);
      vc4_instr.ws(file != REG_A);
      vc4_instr.li_imm = li.imm.encode();
      vc4_instr.sf(instr.set_cond().flags_set());
    }
    break;

    case BR:  // Branch
      assertq(!instr.branch_target().useRegOffset, "Register offset not yet supported");

      vc4_instr.tag(vc4::Instr::BR);
      vc4_instr.cond_add = instr.branch_cond().encode();
      vc4_instr.rel(instr.branch_target().relative);
      vc4_instr.li_imm = 8*instr.branch_target().immOffset;
    break;

    case ALU: {
      auto &alu = instr.ALU;

      RegTag file;
      uint32_t dest = vc4::Instr::encodeDestReg(instr.dest(), &file);

      if (alu.op.isMul()) {
        vc4_instr.cond_mul  = instr.assign_cond().encode();
        vc4_instr.waddr_mul = dest;
        vc4_instr.ws(file != REG_B);
      } else {
        vc4_instr.cond_add  = instr.assign_cond().encode();
        vc4_instr.waddr_add = dest;
        vc4_instr.ws(file != REG_A);
      }

      vc4_instr.sf(instr.set_cond().flags_set());


      if (alu.op.isRot()) {
        assert(alu.srcA.is_reg() && alu.srcA.reg().tag == ACC && alu.srcA.reg().regId == 0);
        assert(!alu.srcB.is_reg() || (alu.srcB.reg().tag == ACC && alu.srcB.reg().regId == 5));
        uint32_t raddrb = 48;

        if (!alu.srcB.is_reg()) {  // i.e. value is an imm
          uint32_t n = alu.srcB.encode();
          assert(n >= 1 || n <= 15);
          raddrb += n;
        }


        vc4_instr.tag(vc4::Instr::ROT);
        vc4_instr.mulOp  = ALUOp(ALUOp::M_V8MIN).vc4_encodeMulOp();
        vc4_instr.raddrb = raddrb;
      } else {
        vc4_instr.tag(vc4::Instr::ALU, instr.hasImm());
        vc4_instr.mulOp = (alu.op.isMul() ? alu.op.vc4_encodeMulOp() : 0);
        vc4_instr.addOp = (alu.op.isMul() ? 0 : alu.op.vc4_encodeAddOp());
        encode_operands(vc4_instr, alu.srcA, alu.srcB);
      }
    }
    break;

    // Halt
    case END:
      vc4_instr.tag(vc4::Instr::END);
      break;

    case RECV: {
      assert(instr.dest() == Reg(ACC,4));  // ACC4 is the only value allowed as dest
      vc4_instr.tag(vc4::Instr::LDTMU);
    }
    break;

    // Semaphore increment/decrement
    case SINC:
      vc4_instr.tag(vc4::Instr::SINC);
      vc4_instr.sema_id = instr.semaId;
      break;

    case SDEC:
      vc4_instr.tag(vc4::Instr::SDEC);
      vc4_instr.sema_id = instr.semaId;
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
