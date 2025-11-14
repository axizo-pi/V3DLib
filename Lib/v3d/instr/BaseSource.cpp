#include "BaseSource.h"
#include "Support/basics.h"

namespace V3DLib {
namespace v3d {
namespace instr {

namespace {

const int V3D_QPU_MUX_R5 = 5;   // From qpu+instr.h. The mux values <= this one aare the same

bool compare_src_dst(BaseSource const &src, BaseSource const &dst) {
  assert(src.is_set() && dst.is_set());

  if (src.is_small_imm()) return false;
  if (src.is_reg() == dst.is_reg()) {  // for both rf and reg
    return src.val() == dst.val();
  }

  return false;
}


}  // anon namespace

bool BaseSource::operator==(const BaseSource &rhs) const {
  assert(m_is_set && rhs.m_is_set);

  if (m_is_dst) {
    if (rhs.m_is_dst) {
      return (m_is_magic == rhs.m_is_magic && m_val == rhs.m_val);
    } else {
      return compare_src_dst(rhs, *this);
    }
  } {
    // This is src
    if (rhs.m_is_dst) {
      return compare_src_dst(*this, rhs);
    } else {
      // Just compare all relevant fields
      bool ret = (this->m_val          == rhs.m_val)
              && (this->m_is_small_imm == rhs.m_is_small_imm)
              && (this->m_is_reg       == rhs.m_is_reg)
      ;
      // m_rfa can be ignored on comparison

      return ret;
    }
  }
}


void BaseSource::set_from_src(uint8_t val, bool is_small_imm, bool is_reg, bool is_rfa) {
  m_is_set       = true;
  m_val          = val;
  m_is_small_imm = is_small_imm;
  m_is_reg       = is_reg;
  m_is_rfa       = is_rfa;
}


void BaseSource::set_from_dst(uint8_t val, bool is_magic) {
  m_is_set   = true;
	m_is_dst   = true;
  m_is_magic = is_magic;
  m_val = val;

  if (is_magic && val <= V3D_QPU_MUX_R5) {
    m_is_reg = true;
  }
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
    } else if (m_is_reg) {
      ret << "Reg: ";
    } else if (m_is_rfa) {
      ret << "rfa: ";
    } else {
      ret << "rfb: ";
    }
  }

  ret << m_val;
  return ret;
}


}  // namespace instr
}  // namespace v3d
}  // namespace V3DLib


