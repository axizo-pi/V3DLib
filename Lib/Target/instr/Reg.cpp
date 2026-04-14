#include "Reg.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Mnemonics.h"

namespace V3DLib {
namespace {

char const *specialStr(RegId rid) {
  switch ((Special) rid) {
    case SPECIAL_UNIFORM:        return "UNIFORM";
    case SPECIAL_ELEM_NUM:       return "ELEM_NUM";
    case SPECIAL_QPU_NUM:        return "QPU_NUM";
    case SPECIAL_RD_SETUP:       return "RD_SETUP";
    case SPECIAL_WR_SETUP:       return "WR_SETUP";
    case SPECIAL_DMA_ST_ADDR:    return "DMA_ST_ADDR";
    case SPECIAL_DMA_LD_ADDR:    return "DMA_LD_ADDR";
    case SPECIAL_DMA_ST_WAIT:    return "DMA_ST_WAIT";
    case SPECIAL_DMA_LD_WAIT:    return "DMA_LD_WAIT";
    case SPECIAL_VPM_READ:       return "VPM_READ";
    case SPECIAL_VPM_WRITE:      return "VPM_WRITE";
    case SPECIAL_MUTEX_ACQUIRE:  return "MUTEX_ACQUIRE";
    case SPECIAL_MUTEX_RELEASE:  return "MUTEX_RELEASE";
    case SPECIAL_HOST_INT:       return "HOST_INT";
    case SPECIAL_TMUA:           return "TMUA";
    case SPECIAL_SFU_RECIP:      return "SFU_RECIP";
    case SPECIAL_SFU_RECIPSQRT:  return "SFU_RECIPSQRT";
    case SPECIAL_SFU_EXP:        return "SFU_EXP";
    case SPECIAL_SFU_LOG:        return "SFU_LOG";
    case SPECIAL_TMUAU:          return "TMUAU";    // vc7 only?
    case SPECIAL_TMUC:           return "TMUC";     // vc7 only?
    case SPECIAL_TMUL:           return "TMUL";     // vc7 only?
  }

  // Unreachable
  assert(false);
  return "";
}


void var_to_reg(Var var, Reg &r) {
  switch (var.tag()) {
    case STANDARD:
      r.tag   = REG_A;
      r.regId = var.id();
      break;
    case UNIFORM:
      r = Target::instr::UNIFORM_READ;
      r.isUniformPtr = var.is_uniform_ptr();
      break;

    case QPU_NUM:       r = Target::instr::QPU_ID;        break;
    case ELEM_NUM:      r = Target::instr::ELEM_ID;       break;
    case VPM_READ:      r = Target::instr::VPM_READ;      break;
    case VPM_WRITE:     r = Target::instr::VPM_WRITE;     break;
    case MUTEX_ACQUIRE: r = Target::instr::MUTEX_ACQUIRE; break;
    case MUTEX_RELEASE: r = Target::instr::MUTEX_RELEASE; break;
    case TMU0_ADDR:     r = Target::instr::TMUA;          break;

    case DUMMY:
      r.tag   = NONE;
      r.regId = var.id();
      break;

    default:
      assertq("srcReg(): Unhandled Var-tag");
      break;
  }
}

}  //  anon namespace


Reg::Reg(Var var) { var_to_reg(var, *this); }


bool Reg::operator==(Reg const &rhs) const {
  // NONE never equals
  if (tag == NONE || rhs.tag == NONE) return false;

  // Comparison field isUniformPtr intentionially skipped here 
  return tag == rhs.tag && regId == rhs.regId;
}


bool Reg::operator<(Reg const &rhs) const {
  if (tag != rhs.tag) {
    return tag < rhs.tag;
  }    

  return regId < rhs.regId;
}


/**
 * Determine reg file of current register
 */
RegTag Reg::regfile() const {
  if (!Platform::compiling_for_vc4()) return REG_A;  // There is no REG_B for v3d, only REG_A

  if (tag > SPECIAL) {
    Log::cerr << "Reg::regfile() invalid tag: " << dump();
  }
  assert(tag <= SPECIAL);

  if (tag == REG_A) return REG_A;
  if (tag == REG_B) return REG_B;

  if (tag == SPECIAL) {
    switch(regId) {
    case SPECIAL_ELEM_NUM:
    case SPECIAL_RD_SETUP:
    case SPECIAL_DMA_LD_WAIT:
    case SPECIAL_DMA_LD_ADDR:
      return REG_A;

    case SPECIAL_QPU_NUM:
    case SPECIAL_WR_SETUP:
    case SPECIAL_DMA_ST_WAIT:
    case SPECIAL_DMA_ST_ADDR:
      return REG_B;

    default:
      break;
    }
  }

  return NONE;
}


bool Reg::is_none() const {
  return *this == Target::instr::None;
}


bool Reg::can_read(bool check) const {
  bool ret = true;

  if (tag == SPECIAL) {
    if (
      regId == SPECIAL_VPM_WRITE ||
      regId == SPECIAL_TMUA ||
      regId == SPECIAL_TMUC
    ) {
      ret = false;
    }
  }

  if (!ret && check) {
    std::string msg = "Can not read from register ";
    msg << dump();
    assertq(msg);
  }

  return ret;
}


bool Reg::can_write(bool check) const {
  bool ret = true;

  if (tag == SPECIAL) {
    if (regId == SPECIAL_UNIFORM || regId == SPECIAL_QPU_NUM
     || regId == SPECIAL_ELEM_NUM || regId == SPECIAL_VPM_READ) {
      ret = false;
    }
  }

  if (!ret && check) {
    std::string msg = "Can not write to register ";
    msg << dump();
    assertq(msg);
  }

  return ret;
}


std::string Reg::dump() const {
  std::string ret;

  switch (tag) {
    case REG_A:   ret <<   "A" << regId; break;
    case REG_B:   ret <<   "B" << regId; break;
    case ACC:     ret << "ACC" << regId; break;
    case SPECIAL: ret <<  "S[" << specialStr(regId) << "]"; break;
    case NONE:    ret <<   "_"; break;

    default: assertq("Reg::dump() failed"); break;
  }

  return ret;
}


// TODO Move this away, to DMA
bool is_dma_only_register(Reg const &reg) {
  if (reg.tag != SPECIAL) return false;

  switch (reg.regId) {
    case SPECIAL_VPM_READ:
    case SPECIAL_DMA_ST_WAIT:
    case SPECIAL_DMA_LD_WAIT:
    case SPECIAL_RD_SETUP:
    case SPECIAL_WR_SETUP:
    case SPECIAL_HOST_INT:
    case SPECIAL_DMA_LD_ADDR:
      return true;
  }

  return false;
}

}  // namespace V3DLib
