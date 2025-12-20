#ifndef _V3DLIB_VC4_INSTR_H_
#define _V3DLIB_VC4_INSTR_H_
#include <stdint.h>

namespace V3DLib {
namespace vc4 {

class Instr {
public:
  enum Tag {
    NOP,
    ROT,
    ALU,
    LI,
    BR,
    END,
    SINC,
    SDEC,
    LDTMU  // Always writes to ACC4
  };

  uint32_t cond_add = 0;    // reused for LI, BR
  uint32_t cond_mul = 0;
  uint32_t waddr_add = 39;  // reused for LI
  uint32_t waddr_mul = 39;

  uint32_t addOp  = 0;
  uint32_t mulOp  = 0;
  uint32_t muxa   = 0;
  uint32_t muxb   = 0;
  uint32_t raddra = 39;
  uint32_t raddrb = 0;

  uint32_t li_imm = 0;  // Also used as BR target
  uint32_t sema_id = 0;

  void sf(bool val);
  void ws(bool val);
  void rel(bool val);

  void tag(Tag in_tag, bool imm = false);

  uint64_t encode() const {
   return (((uint64_t) high()) << 32) + low();
  }

private:
  Tag m_tag = NOP;
  uint32_t m_sig = 14;        // 0xe0000000
  uint32_t m_sem_flag = 0;    // TODO research what this is for, only used with SINC/SDEC

  bool m_ws  = false;
  bool m_sf  = false;
  bool m_rel = false;

  uint32_t high() const;
  uint32_t low() const;
};

}  // namespace vc4
}  // namespace V3DLib

#endif  // _V3DLIB_VC4_INSTR_H_
