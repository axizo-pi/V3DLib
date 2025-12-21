#include "Instr.h"
#include "Support/basics.h"  // fatal()
#include "global/log.h"

using namespace Log;

namespace V3DLib {
namespace {

/**
 * Handle case where there are two source operands
 *
 * This is fairly convoluted stuff; apparently there are rules with regfile A/B usage
 * which I am not aware of.
 */
void encode_operands(vc4::Instr &instr, RegOrImm const &srcA, RegOrImm const &srcB) {
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
      muxa   = vc4::Instr::MUX_B;
      muxb   = vc4::Instr::MUX_B;
    } else if (srcB.is_imm()) {
      // Second operand is a small immediate
      raddr_a = vc4::Instr::encodeSrcReg(srcA.reg(), REG_A, muxa);
      raddr_b = srcB.encode();
      muxb   = vc4::Instr::MUX_B;
    } else if (srcA.is_imm()) {
      // First operand is a small immediate
      raddr_a = vc4::Instr::encodeSrcReg(srcB.reg(), REG_A, muxb);
      raddr_b = srcA.encode();
      muxa   = vc4::Instr::MUX_B;
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

} // anon namespace


namespace vc4 {

uint32_t Instr::high() const {
  uint32_t ret = 0;

  switch (enc) {
    case NONE:                   assert(false);           break; 
    case ALU:                    assert(SOFTWARE_BREAKPOINT < sig && sig <= BRANCH);
                                 ret |= (sig       << 28); break;
    case ALU_SMALL_IMM:          ret |= (0b1101    << 28); break;
    case BRANCH_ENC:             ret |= (0b1111    << 28); break;
    case LOAD_IMM:               ret |= (0b1110000 << 24); break;
    case LOAD_IMM_ELEM_SIGNED:   ret |= (0b1110001 << 24); break;
    case LOAD_IMM_ELEM_UNSIGNED: ret |= (0b1110011 << 24); break;
    case SEMAPHORE:              ret |= (0b1110100 << 24); break;
  }

  assert(waddr_add < 64);
  assert(waddr_mul < 64);

  ret |= (ws ?(1 << 12):0) 
      |  (waddr_add <<  6) | (waddr_mul);

  if (sig == BRANCH) {
    assert(raddr_a < 64);

    ret |= (cond_br << 20)
        |  (rel ?(1 << 19):0) 
        |  (reg ?(1 << 18):0) 
        |  (raddr_a << 13);
  } else {
    assert(cond_add < 8);
    assert(cond_mul < 8);
    assert(pack < PACK_SIZE);

    ret |= (pm ?(1 << 24):0)
        |  (pack      << 20)
        |  (cond_add  << 17) | (cond_mul << 14)
        |  (sf ?(1 << 12):0) ;
  }


  if (enc == ALU || enc == ALU_SMALL_IMM) {
    assert(unpack < UNPACK_SIZE);
    ret |= (unpack << 25);
  }

  return ret;
}


uint32_t Instr::low() const {
  //assert(sig > SOFTWARE_BREAKPOINT);
  assert(enc != NONE);

  uint32_t ret = 0;

  if (enc == ALU || enc == ALU_SMALL_IMM) {
    assert(add_a < 8);
    assert(add_b < 8);
    assert(mul_a < 8);
    assert(mul_b < 8);
    assert(raddr_a < 64);
    assert(op_mul < 8);
    assert(op_add < 32);

    ret |= (add_a   << 9)  | (add_b  <<  6) | (mul_a << 3) | (mul_b)
        |  (op_mul  << 29) | (op_add << 24)
        |  (raddr_a << 18) | (raddr_b << 12);
  }

  if (enc == ALU ||enc == ALU_SMALL_IMM) {
    assert(raddr_b < 64);
    ret |= (raddr_b << 12);
  }

  if ((enc == BRANCH_ENC) || enc == LOAD_IMM) {
    ret |= (immediate);
  }

  return ret;
}


uint64_t Instr::encode() const {
 return (((uint64_t) high()) << 32) + low();
}


/**
 * @brief Determine the regfile and index combination to use for writes, for the passed 
 *        register definition 'reg'.
 *
 * This function deals exclusively with write values of the regfile registers.
 *
 * See also NOTES in header comment for `encodeSrcReg()`.
 *
 * @param reg   register definition for which to determine output value
 * @param file  out-parameter; the regfile to use (either REG_A or REG_B)
 *
 * @return index into regfile (A, B or both) of the passed register
 *
 * ----------------------------------------------------------------------------------------------
 * ## NOTES
 *
 * * The regfile location for `ACC4` is called `TMP_NOSWAP` in the doc. This is because
 *   special register `r4` (== ACC4) is read-only.
 *   TODO: check code if ACC4 is actually used. Better to name it TMP_NOSWAP
 *
 * * ACC5 has parentheses with extra function descriptions.
 *   This implies that the handling of ACC5 differs from the others (at least, for ACC[0123])
 *   TODO: Check code for this; is there special handling of ACC5? 
 */
uint8_t Instr::encodeDestReg(Reg reg, RegTag* file) {
  // Selection of regfile for the cases where using A or B doesn't matter
  RegTag AorB = REG_A;  // Select A as default
  if (reg.tag == REG_A || reg.tag == REG_B) {
    AorB = reg.tag;     // If the regfile is preselected in `reg`, use that.
  }

  switch (reg.tag) {
    case REG_A:
      assert(reg.regId >= 0 && reg.regId < NUM_RF);
      *file = REG_A;
      return (uint8_t) reg.regId;

    case REG_B:
      assert(reg.regId >= 0 && reg.regId < NUM_RF);
      *file = REG_B;
      return (uint8_t) reg.regId;

    case ACC:
      // See NOTES in header comment
      assert(reg.regId >= 0 && reg.regId <= 5); // !! ACC4 is TMP_NOSWAP, *not* r4
      *file = reg.regId == 5 ? REG_B : AorB; 
      return (uint8_t) (ACC_START + reg.regId);

    case SPECIAL:
      switch (reg.regId) {
        case SPECIAL_RD_SETUP:      *file = REG_A; return VPMVCD_RD_SETUP;
        case SPECIAL_WR_SETUP:      *file = REG_B; return VPMVCD_WR_SETUP;
        case SPECIAL_DMA_LD_ADDR:   *file = REG_A; return VPM_LD_ADDR;
        case SPECIAL_DMA_ST_ADDR:   *file = REG_B; return VPM_ST_ADDR;
        case SPECIAL_VPM_WRITE:     *file = AorB;  return VPM_WRITE;
        case SPECIAL_HOST_INT:      *file = AorB;  return HOST_INT;
        case SPECIAL_TMUA:          *file = AorB;  return TMU0_S;
        case SPECIAL_SFU_RECIP:     *file = AorB;  return SFU_RECIP;
        case SPECIAL_SFU_RECIPSQRT: *file = AorB;  return SFU_RECIPSQRT;
        case SPECIAL_SFU_EXP:       *file = AorB;  return SFU_EXP;
        case SPECIAL_SFU_LOG:       *file = AorB;  return SFU_LOG;
        default:                    break;
      }

    case RegTag::NONE:
      // NONE maps to 'NOP' in regfile.
      *file = AorB;
      return NOP_R;

    default:
      fatal("V3DLib: missing case in encodeDestReg");
      return 0;
  }
}


/**
 * @brief Determine regfile index and the read field encoding for alu-instructions, for the passed 
 *        register 'reg'.
 *
 * The read field encoding, output parameter `mux` is a bitfield in instructions `alu` and 
 * 'alu small imm'. It specifies the register(s) to use as input.
 *
 * This function deals exclusively with 'read' values.
 *
 * @param reg   register definition for which to determine output value
 * @param file  regfile to use. This is used mainly to validate the `regid` field in param `reg`. In specific
 *              cases where both regfile A and B are valid (e.g. NONE), it is used to select the regfile.
 * @param mux   out-parameter; value in ALU instruction encoding for fields `add_a`, `add_b`, `mul_a` and `mul_b`.
 *
 * @return index into regfile (A, B or both) of the passed register.
 *
 * ----------------------------------------------------------------------------------------------
 * ## NOTES
 *
 * * There are four combinations of access to regfiles:
 *   - read A
 *   - read B
 *   - write A
 *   - write B
 *
 * This is significant, because SPECIAL registers may only be accessible through a specific combination
 * of A/B and read/write.
 *
 * * References in VideoCore IV Reference document:
 *
 *   - Fields `add_a`, `add_b`, `mul_a` and `mul_b`: "Figure 4: ALU Instruction Encoding", page 26
 *   - mux value: "Table 3: ALU Input Mux Encoding", page 28
 *   - Index regfile: "Table 14: 'QPU Register Addess Map'", page 37.
 *
 * ----------------------------------------------------------------------------------------------
 * ## TODO
 *
 * * NONE/NOP - There are four distinct versions for `NOP`, A/B and no read/no write.
 *              Verify if those distinctions are important at least for A/B.
 *              They might be the same thing.
 */
uint8_t Instr::encodeSrcReg(Reg reg, RegTag file, uint8_t &mux) {
  assert (file == REG_A || file == REG_B);

  const uint8_t NO_REGFILE_INDEX = 0;  // Return value to use when there is no regfile mapping for the register

  // Selection of regfile for the cases that both A and B are possible.
  // Note that param `file` has precedence here.
  uint8_t AorB = (file == REG_A)? MUX_A: MUX_B;

  switch (reg.tag) {
    case REG_A:
      assert(reg.regId >= 0 && reg.regId < NOP_R && file == REG_A);
      mux = MUX_A;
      return (uint8_t) reg.regId;

    case REG_B:
      assert(reg.regId >= 0 && reg.regId < NOP_R && file == REG_B);
      mux = MUX_B;
      return (uint8_t) reg.regId;

    case ACC:
      // ACC does not map onto a regfile for 'read'
      assert(reg.regId >= 0 && reg.regId <= 4);  // TODO index 5 missing, is this correct??
      mux = (uint8_t) reg.regId;
      return NO_REGFILE_INDEX;

    case RegTag::NONE:
      // NONE maps to `NOP` in the regfile
      mux = AorB;
      return NOP_R;

    case SPECIAL:
      switch (reg.regId) {
        case SPECIAL_UNIFORM:     mux = AorB;                         return UNIFORM_READ;
        case SPECIAL_ELEM_NUM:    assert(file == REG_A); mux = MUX_A; return ELEMENT_NUMBER;
        case SPECIAL_QPU_NUM:     assert(file == REG_B); mux = MUX_B; return QPU_NUMBER;
        case SPECIAL_VPM_READ:    mux = AorB;                         return VPM_READ;
        case SPECIAL_DMA_LD_WAIT: assert(file == REG_A); mux = MUX_A; return VPM_LD_WAIT;
        case SPECIAL_DMA_ST_WAIT: assert(file == REG_B); mux = MUX_B; return VPM_ST_WAIT;
      }

    default:
      fatal("V3DLib: missing case in encodeSrcReg");
      return 0;
  }
}


void Instr::encode(Target::Instr const &instr) {

  switch (instr.tag) {
    case Target::ALU: {
      auto &alu = instr.ALU;

      RegTag file;
      uint8_t dest = encodeDestReg(instr.dest(), &file);

      if (alu.op.isMul()) {
        cond_mul  = instr.assign_cond().encode();
        waddr_mul = dest;
        ws        = (file != REG_B);
      } else {
        cond_add  = instr.assign_cond().encode();
        waddr_add = dest;
        ws        = (file != REG_A);
      }

      sf = (instr.set_cond().flags_set());

      if (alu.op.isRot()) {
        breakpoint;
        assert(alu.srcA.is_reg() && alu.srcA.reg().tag == ACC && alu.srcA.reg().regId == 0);
        assert(!alu.srcB.is_reg() || (alu.srcB.reg().tag == ACC && alu.srcB.reg().regId == 5));
        uint8_t _raddr_b = 48;

        if (!alu.srcB.is_reg()) {  // i.e. value is an imm

// Grumbl mofo compiler insists that n is int
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
          uint8_t n = alu.srcB.encode();
          assert(n >= 1 || n <= 15);
          _raddr_b += n;
#pragma GCC diagnostic pop

        }

        enc     = ALU_SMALL_IMM;
        op_mul  = ALUOp(Enum::M_V8MIN).vc4_encodeMulOp();
        raddr_b = _raddr_b;
      } else {
        if (instr.hasImm()) {
          enc  = ALU_SMALL_IMM;
        } else {
          enc  = ALU;
        }

        op_mul = (alu.op.isMul() ? alu.op.vc4_encodeMulOp() : 0);
        op_add = (alu.op.isMul() ? 0 : alu.op.vc4_encodeAddOp());
        encode_operands(*this, alu.srcA, alu.srcB);
      }
    }
    break;

    case Target::LI: {
      auto &li = instr.LI;
      RegTag file;

      enc       = vc4::Instr::LOAD_IMM;
      cond_add  = instr.assign_cond().encode();
      waddr_add = vc4::Instr::encodeDestReg(instr.dest(), &file);
      ws        = (file != REG_A);
      immediate = li.imm.encode();
      sf        = instr.set_cond().flags_set();

      breakpoint;
    }
    break;

    default:
      // Deal with other options as they come
      warn << "Target tag not dealt with yet, examine";
      breakpoint;
      break;
  }

  warn << instr.dump();
  warn << dump();
}


std::string Instr::dump() const {
  std::string ret;

  auto mux_to_str = [*this] (uint8_t mux) -> std::string {
    std::string ret;

    if (mux == MUX_A) {
      if (raddr_a == 32) {
        ret << "unif";
      } else if (raddr_a == 38) {
        ret << "elem_num";
      } else {
        ret << "rfa" << raddr_a;
      }
    } else if (mux == MUX_B) {
      if (enc == ALU_SMALL_IMM) {
        // This should actually be converted to representation value
        ret << raddr_b;
      } else {
        if (raddr_b == 32) {
          ret << "unif";
        } else if (raddr_b == 38) {
          ret << "qpu_num";
        } else {
          ret << "rfb" << raddr_b;
        }
      }
    } else {
      ret << "a" << mux;
    }

    return ret;
  };


  auto waddr_to_str = [*this] (uint8_t waddr, bool do_a) -> std::string {
    std::string ret;

    if (ACC_START <= waddr && waddr <= ACC_END) {
      ret << "a" << (waddr - ACC_START);
      return ret;
    }

    if (waddr == 32) {
      ret << "unif";
      return ret;
    }

    if (do_a) {
      if (ws) {
        ret << "rfb";
      } else {
        ret << "rfa";
      }
    } else {
      if (ws) {
        ret << "rfa";
      } else {
        ret << "rfb";
      }
    }

    ret << waddr; 

    return ret;
  };

  ret << "enc: ";
  switch (enc) {
  case NONE:          ret << "NONE";          break;
  case ALU:           ret << "ALU";           break;
  case ALU_SMALL_IMM: ret << "ALU_SMALL_IMM"; break;
  case LOAD_IMM:      ret << "LOAD_IMM";      break;

  default:
    ret << enc;
    break;
  }

  if (enc == LOAD_IMM) {
    ret << " " << immediate;
  } else {
    bool found_it;
    std::string op;

    if (op_add != 0) {
      op = dump_add_op(op_add, found_it);
      ret << " add: " << op << "("
          << waddr_to_str(waddr_add, true) << ", "
          << mux_to_str(add_a)             << ", "
          << mux_to_str(add_b)             << ")";

    } else {
      ret << " op_add: NOP";
    }

    if (op_mul != 0) {
      //op = dump_add_op(op_add, found_it);
      ret << " mul: TODO" << "("
          << waddr_to_str(waddr_mul, false) << ", "
          << mux_to_str(mul_a)              << ", "
          << mux_to_str(mul_b)              << ")";
    } else {
      ret << " mul: NOP";
    }
  }

  if (sig != NO_SIGNAL)    ret << " sig: " << sig;
  if (unpack != NO_UNPACK) ret << " unpack: " << unpack; // Specific for ALU, ALU_IMMEDIATE
  if (pack != NO_PACK)     ret << " pack: " << pack;

  if (pm)  ret << " pm";
  if (sf)  ret << " sf";
  if (ws)  ret << " ws";
  if (rel) ret << " rel";
  if (reg) ret << " reg";
  if (sa)  ret << " sa";


/*
  uint8_t cond_add = 0;
  uint8_t cond_mul = 0;


  // Specific for branch
  BranchCondition cond_br = ALWAYS;

  uint8_t waddr_add = NOP_R;
  uint8_t waddr_mul = NOP_R;

  uint8_t op_mul = 0;
  uint8_t op_add = 0;

  uint8_t raddr_a = NOP_R;
  uint8_t raddr_b = NOP_R;

  // specific for ALU_IMMEDIATE
  uint8_t small_imm = 0;

  uint32_t immediate = 0;

  uint32_t semaphore = 0;
*/
  return ret;
}

}  // namespace vc4
}  // namespace V3DLib
