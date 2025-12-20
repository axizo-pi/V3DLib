#include "Instr.h"
#include <cassert>

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
      break;  // as is

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
      m_sig = 3;
      raddrb = 39;
      break;

    case LDTMU:
      m_sig = 10;
      raddrb = 39;
      break;

    case SINC:
    case SDEC:
      m_sig = 0xe;
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

}  // namespace vc4
}  // namespace V3DLib
