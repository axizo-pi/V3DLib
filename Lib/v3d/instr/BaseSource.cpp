#include "BaseSource.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Instr.h"

namespace V3DLib {
namespace v3d {
namespace instr {

using ::operator<<;  // C++ weirdness; come on c++, get a grip.

namespace {

const int V3D_QPU_MUX_R5 = 5;   // From qpu.instr.h. The mux values <= this one are the same

}  // anon namespace


BaseSource::BaseSource(Source const &rhs) {
  // Not expecting dst register right now

  if (rhs.is_small_imm()) {
    m_is_small_imm = true;
    m_val = rhs.small_imm().val();
  } else {
    // It's a location
    _init(rhs.location());
  }

  m_is_set   = true;
  unpack(rhs.input_unpack());
}


BaseSource::BaseSource(Location const &rhs) {
  _init(rhs);
  m_is_set   = true;
}


void BaseSource::_init(Location const &rhs) {
  assert (!Platform::compiling_for_vc7() || rhs.is_rf());

  m_val    = rhs.to_waddr();
   m_is_rf  = rhs.is_rf();
  m_is_reg = rhs.is_reg();

  unpack(rhs.input_unpack());
}


BaseSource::BaseSource(Instr const &instr, int check_src) {
  assert(check_src <= CheckSrc::CHECK_MUL_B);

  v3d_qpu_input input;
  bool small_imm = false;  // for vc7

  // Ugly but necessary
  switch (check_src) {
    case CHECK_ADD_A:
      if (instr.add_nop()) return;
      if (!instr.alu_add_a_set()) return;
      input = instr.alu.add.a;
      small_imm = instr.sig.small_imm_a;
    break;

    case CHECK_ADD_B:
      if (instr.add_nop()) return;
      if (!instr.alu_add_b_set()) return;
      input = instr.alu.add.b;
      small_imm = instr.sig.small_imm_b;
    break;

    case CHECK_MUL_A:
      if (instr.mul_nop()) return;
      if (!instr.alu_mul_a_set()) return;
      input = instr.alu.mul.a;
      small_imm = instr.sig.small_imm_c;
    break;

    case CHECK_MUL_B:
      if (instr.mul_nop()) return;
      if (!instr.alu_mul_b_set()) return;
      input = instr.alu.mul.b;
      small_imm = instr.sig.small_imm_d;
    break;

    default: assert(false);
  }

  if (Platform::compiling_for_vc7()) {
    // vc7 - no acc's
    set_from_src(input.raddr, small_imm, false, true);
  } else {
    // 
    if (input.mux == V3D_QPU_MUX_A) {
      set_from_src(instr.raddr_a, false, false, true );
    } else if (input.mux == V3D_QPU_MUX_B) {
      set_from_src(instr.raddr_b, instr.sig.small_imm_b, false, true );
    } else {
      set_from_src(input.mux, false, true, false);
    }
  }

  unpack(input.unpack);
}


bool BaseSource::operator==(const Register &rhs) const {
  if (!m_is_set) return false;  // Empty item can never be equal

  if (m_is_small_imm) return false;
  if (m_is_reg && rhs.to_waddr() <= V3D_QPU_WADDR_R5) return m_val == rhs.to_waddr();
  if (m_is_magic) return m_val == rhs.to_waddr();

  return false;
}

bool BaseSource::operator==(const BaseSource &rhs) const {
  if (!m_is_set || !rhs.m_is_set) return false;  // Empty items can never be equal

  if (m_is_small_imm != rhs.m_is_small_imm) return false;
  if (m_is_reg != rhs.m_is_reg) return false;
  if (m_is_magic != rhs.m_is_magic) return false;

  return m_val == rhs.m_val;
}

/**
 * Needed for std::set
 */
bool BaseSource::operator<(const BaseSource &rhs) const {
  if (m_is_set != rhs.m_is_set) return m_is_set < rhs.m_is_set;
  if (m_is_small_imm != rhs.m_is_small_imm) return m_is_small_imm < rhs.m_is_small_imm;
  if (m_is_reg != rhs.m_is_reg) return m_is_reg < rhs.m_is_reg;
  if (m_is_magic != rhs.m_is_magic) return m_is_magic < rhs.m_is_magic;

  return (m_val < rhs.m_val);
}


void BaseSource::set_from_src(uint8_t val, bool is_small_imm, bool is_reg, bool is_rf) {
  m_is_set       = true;
  m_val          = val;
  m_is_small_imm = is_small_imm;
  m_is_reg       = is_reg;

  if (!is_small_imm) {
    m_is_rf        = is_rf;
  } else {
    assert(!m_is_rf);
  }
}


void BaseSource::set_from_dst(uint8_t val, bool is_magic) {
  m_is_set   = true;
  m_is_dst   = true;
  m_is_magic = is_magic;
  m_val = val;

  if (is_magic && val <= V3D_QPU_MUX_R5) {
    m_is_reg   = true;
    m_is_magic = false;
  }
}


bool BaseSource::is_magic() const {
  if (!m_is_set) return false;
  return m_is_magic;
}


std::string BaseSource::dump() const {
  std::string ret;

  if (!m_is_set) {
    ret << "Not set";
  } else if (m_is_dst) {
    ret << "Dst ";

    if (m_is_magic) {
      ret << "magic ";
    }

    ret << m_val;

  } else {
    ret << "Src ";

    if (m_is_small_imm) {
      ret << "Small imm: ";
    }  else if (m_is_reg) {
       ret << "r";
    } else if (m_is_rf) {
      ret << "rf";
    } else {
      ret << "unknown ";
    }

    ret << m_val;
  }

  return ret;
}


bool BaseSource::uses_global_raddr() const {
  if (Platform::compiling_for_vc7()) return false;

  if (!m_is_set)      return false;
  if (m_is_small_imm) return true;
  if (m_is_reg)       return false;
//  if (m_is_dst)       return false;
  if (m_is_magic)     return false;

  return true;
}

}  // namespace instr
}  // namespace v3d
}  // namespace V3DLib


