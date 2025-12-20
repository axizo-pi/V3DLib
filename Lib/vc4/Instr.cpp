#include "Instr.h"
#include "Support/basics.h"  // fatal()
//#include <cassert>

namespace V3DLib {
namespace vc4 {

void Instr::sf(bool val) { assert(m_tag != BR); m_sf = val; }
void Instr::ws(bool val) { assert(m_tag != BR); m_ws = val; }
void Instr::rel(bool val) { assert(m_tag == BR); m_rel = val; }


void Instr::tag(Tag in_tag, bool imm) {
  m_tag = in_tag;

  switch(m_tag) {
    case NOP:  // default
    case LI:
      break;   // as is

    case ROT:
      m_sig = 13;
      break;

    case ALU:
      m_sig = imm?13:1;
      break;

    case BR:
      m_sig = 15;
      break;

    case END:
      m_sig  = 3;
      raddrb = NOP_R;
      break;

    case LDTMU:
      m_sig  = 10;
      raddrb = NOP_R;
      break;

    case SINC:
    case SDEC:
      m_sig      = 0xe;
      m_sem_flag = 8;
      break;
  };
}


uint32_t Instr::high() const {
  uint32_t ret = (m_sig << 28) | (m_sem_flag << 24) | (waddr_add << 6) | waddr_mul;

  if (m_tag == BR) {
    ret |= (cond_add << 20)
        |  (m_rel?(1 << 19):0);
  } else if (m_tag != END && m_tag != LDTMU) {
    ret |= (cond_add << 17) | (cond_mul << 14) 
        |  (m_sf?(1 << 13):0)
        |  (m_ws?(1 << 12):0);
  }

  return ret;
}


uint32_t Instr::low() const {
  switch (m_tag) {
    case NOP:
      return  0;

    case ROT:
      return (mulOp << 29) | (raddra << 18) | (raddrb << 12);

    case ALU:
      return (mulOp << 29) | (addOp << 24) | (raddra << 18) | (raddrb << 12)
            | (muxa << 9) | (muxb << 6)
            | (muxa << 3) | muxb;

    case END:
    case LDTMU:
      return (raddra << 18) | (raddrb << 12);

    case LI:
    case BR:
      return li_imm;

    case SINC:
      return sema_id;
    case SDEC:
      return (1 << 4) | sema_id;
  };

  assert(false);
  return 0;
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
uint32_t Instr::encodeDestReg(Reg reg, RegTag* file) {
  // Selection of regfile for the cases where using A or B doesn't matter
  RegTag AorB = REG_A;  // Select A as default
  if (reg.tag == REG_A || reg.tag == REG_B) {
    AorB = reg.tag;     // If the regfile is preselected in `reg`, use that.
  }

  switch (reg.tag) {
    case REG_A:
      assert(reg.regId >= 0 && reg.regId < NUM_RF);
      *file = REG_A;
      return reg.regId;

    case REG_B:
      assert(reg.regId >= 0 && reg.regId < NUM_RF);
      *file = REG_B;
      return reg.regId;

    case ACC:
      // See NOTES in header comment
      assert(reg.regId >= 0 && reg.regId <= 5); // !! ACC4 is TMP_NOSWAP, *not* r4
      *file = reg.regId == 5 ? REG_B : AorB; 
      return NUM_RF + reg.regId;

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

    case NONE:
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
uint32_t Instr::encodeSrcReg(Reg reg, RegTag file, uint32_t* mux) {
  assert (file == REG_A || file == REG_B);

  const uint32_t NO_REGFILE_INDEX = 0;  // Return value to use when there is no regfile mapping for the register

  // Selection of regfile for the cases that both A and B are possible.
  // Note that param `file` has precedence here.
  uint32_t AorB = (file == REG_A)? MUX_A: MUX_B;

  switch (reg.tag) {
    case REG_A:
      assert(reg.regId >= 0 && reg.regId < NOP_R && file == REG_A);
      *mux = 6;
      return reg.regId;

    case REG_B:
      assert(reg.regId >= 0 && reg.regId < NOP_R && file == REG_B);
      *mux = 7;
      return reg.regId;

    case ACC:
      // ACC does not map onto a regfile for 'read'
      assert(reg.regId >= 0 && reg.regId <= 4);  // TODO index 5 missing, is this correct??
      *mux = reg.regId;
      return NO_REGFILE_INDEX;

    case NONE:
      // NONE maps to `NOP` in the regfile
      *mux = AorB;
      return NOP_R;

    case SPECIAL:
      switch (reg.regId) {
        case SPECIAL_UNIFORM:     *mux = AorB;                         return UNIFORM_READ;
        case SPECIAL_ELEM_NUM:    assert(file == REG_A); *mux = MUX_A; return ELEMENT_NUMBER;
        case SPECIAL_QPU_NUM:     assert(file == REG_B); *mux = MUX_B; return QPU_NUMBER;
        case SPECIAL_VPM_READ:    *mux = AorB;                         return VPM_READ;
        case SPECIAL_DMA_LD_WAIT: assert(file == REG_A); *mux = MUX_A; return VPM_LD_WAIT;
        case SPECIAL_DMA_ST_WAIT: assert(file == REG_B); *mux = MUX_B; return VPM_ST_WAIT;
      }

    default:
      fatal("V3DLib: missing case in encodeSrcReg");
      return 0;
  }
}

}  // namespace vc4
}  // namespace V3DLib
