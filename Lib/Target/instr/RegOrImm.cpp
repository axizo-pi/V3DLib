#include "RegOrImm.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Target/SmallLiteral.h"
#include "Imm.h"
#include "v3d/instr/SmallImm.h"

using namespace Log;

namespace V3DLib {

RegOrImm::RegOrImm(Imm const &rhs) : m_is_reg(false), m_imm(rhs){
  //   set_imm(rhs.encode_imm());
}

RegOrImm::RegOrImm(int rhs) : m_is_reg(false), m_imm(rhs){
}

RegOrImm::RegOrImm(Var const &rhs) { set_reg(rhs); }
RegOrImm::RegOrImm(Reg const &rhs) { set_reg(rhs); }

Reg &RegOrImm::reg()                  { assert(is_reg()); return m_reg; }
Reg RegOrImm::reg() const             { assert(is_reg()); return m_reg; }
Imm &RegOrImm::imm()                  { assert(is_imm()); return m_imm; }
Imm RegOrImm::imm() const             { assert(is_imm()); return m_imm; }

/*
EncodedSmallImm &RegOrImm::imm()      { assert(is_imm()); return m_smallImm; }
EncodedSmallImm RegOrImm::imm() const { assert(is_imm()); return m_smallImm; }
*/

uint32_t RegOrImm::encode() const {
  assert(Platform::compiling_for_vc4() || Platform::running_emulator());
  assert(is_imm());

  //warn << "encode() imm: " << m_imm.dump();

  int ret = m_imm.encode_imm();

  //warn << "encode() ret: " << hex << ret;

  if (ret == -1) {
    warn << "RegOrImm::encode() invalid encoding, imm: " << m_imm.dump();
    breakpoint;
    warn << "Aborting" << thrw;
  }

  // input should be in the encode range for target platforms
  assert(v3d::instr::SmallImm::is_legal_encoded_value(ret));

  return (uint32_t) ret;
}


/*
void RegOrImm::set_imm(int rhs) {
  // input should be in the encode range for target platforms
  assert((Platform::compiling_for_vc4()  && 0 <= rhs && rhs <= 47)
      || (!Platform::compiling_for_vc4() && v3d::instr::SmallImm::is_legal_encoded_value(rhs))
  );

  m_is_reg  = false;
  m_smallImm.val = rhs;
}
*/


void RegOrImm::set_reg(Reg const &rhs) {
  m_is_reg  = true;
  m_reg = rhs;
  m_reg.can_read(true);
}


//RegOrImm &RegOrImm::operator=(int rhs)        { set_imm(rhs); return *this; }


RegOrImm &RegOrImm::operator=(Imm const &rhs) {
  //set_imm(rhs.encode_imm());

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
/*
    if (Platform::compiling_for_vc4()) {
      return printSmallLit(m_smallImm.val);
    } else {
      return v3d::instr::SmallImm::print_encoded_value(m_smallImm.val);
    }
*/
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
