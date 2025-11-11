#include "BaseSource.h"
#include "Support/basics.h"

namespace V3DLib {
namespace v3d {
namespace instr {

void BaseSource::set(uint8_t val, bool is_small_imm, bool is_reg) {
  m_is_set       = true;
  m_val          = val;
  m_is_small_imm = is_small_imm;
  m_is_reg       = is_reg;
}


std::string BaseSource::dump() const {
  std::string ret;

  if (!m_is_set) {
    ret << "Not set";
  } else if (m_is_small_imm) {
    ret << "Small imm: ";
  } else if (m_is_reg) {
    ret << "Reg: ";
  } else {
    ret << "rf: ";
  }

  if (m_is_set) {
    ret << m_val;
  }

  return ret;
}


}  // namespace instr
}  // namespace v3d
}  // namespace V3DLib


