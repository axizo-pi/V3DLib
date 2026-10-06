#include "RegOrImm.h"
#include "v3d/instr/SmallImm.h"
#include "v3d/UniformConstants.h"
#include "Support/basics.h"
#include "Support/Platform.h"

namespace V3DLib {

RegOrImm::RegOrImm(Imm const &rhs) : m_is_reg(false), m_imm(rhs) {}
RegOrImm::RegOrImm(int rhs) : m_is_reg(false), m_imm(rhs) {}
RegOrImm::RegOrImm(Var const &rhs) { set_reg(rhs); }
RegOrImm::RegOrImm(Reg const &rhs) { set_reg(rhs); }

RegOrImm::RegOrImm(float rhs) : m_is_reg(false), m_imm(rhs) {
  if (Platform::compile::for_vc4()) return; 

  Imm dummy(rhs);
  if (dummy.encode_imm() != -1) {
    return;
  }

  int index = v3d::uniform_constants.get(rhs);
  set_reg(Var(STANDARD, index));
}

Reg &RegOrImm::reg()                  { assert(is_reg()); return m_reg; }
Reg RegOrImm::reg() const             { assert(is_reg()); return m_reg; }
Imm &RegOrImm::imm()                  { assert(is_imm()); return m_imm; }
Imm RegOrImm::imm() const             { assert(is_imm()); return m_imm; }


uint8_t RegOrImm::encode() const {
  assert(Platform::emulate::running() || Platform::compile::for_vc4());
  assert(is_imm());

  int ret = m_imm.encode_imm();

  assert(v3d::instr::SmallImm::is_legal_encoded_value(ret));
  assert(ret >= 0);
  return (uint8_t) ret;
}


void RegOrImm::set_reg(Reg const &rhs) {
  m_is_reg  = true;
  m_reg = rhs;
  m_reg.can_read(true);
}


RegOrImm &RegOrImm::operator=(Imm const &rhs) {
  m_imm = rhs;
  m_is_reg = false;

  return *this;
}


RegOrImm &RegOrImm::operator=(Reg const &rhs) { set_reg(rhs); return *this; }


bool RegOrImm::operator==(RegOrImm const &rhs) const {
  if (m_is_reg != rhs.m_is_reg) return false;

  if (m_is_reg) {
    return m_reg == rhs.m_reg;
  } else {
    return m_imm == rhs.m_imm;
  }
}


bool RegOrImm::operator==(Reg const &rhs) const {
  if (!m_is_reg) return false;
  return m_reg == rhs;
}


bool RegOrImm::operator==(Imm const &rhs) const {
  if (m_is_reg) return false;
  return m_imm == rhs;
}


bool RegOrImm::can_read(bool check) const {
  if (m_is_reg) return m_reg.can_read(check);
  return true;
}


std::string RegOrImm::dump() const {
  if (m_is_reg) {
    return m_reg.dump();
  } else {
    return m_imm.dump();
  }
}


bool RegOrImm::is_transient() const {
  if (is_imm()) return false;
  return (reg().tag == NONE || reg().tag == TMP_A || reg().tag == TMP_B);
}


bool RegOrImm::uses_src() const {
  if (is_imm()) return true;
  return (reg().tag == REG_A || reg().tag == REG_B);
}

}  // namespace V3DLib
