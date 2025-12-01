#include "BaseSource.h"
#include "Support/basics.h"
#include "Support/Platform.h"



namespace V3DLib {
namespace v3d {
namespace instr {

namespace {

const int V3D_QPU_MUX_R5 = 5;   // From qpu.instr.h. The mux values <= this one are the same

}  // anon namespace


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
    } else if (!Platform::compiling_for_vc7()) {
			// vc6
			if (m_is_reg) {
      	ret << "Reg: ";
	    } else if (m_is_rfa) {
	      ret << "rfa: ";
	    } else {
	      ret << "rfb: ";
	    }
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


