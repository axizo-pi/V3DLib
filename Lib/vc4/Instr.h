#ifndef _V3DLIB_VC4_INSTR_H_
#define _V3DLIB_VC4_INSTR_H_
#include <stdint.h>
#include "Target/instr/Reg.h"

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

  enum Registers {
    MUX_A           = 6,
    MUX_B           = 7,

    NUM_RF          = 32,
    UNIFORM_READ    = 32,      // rfA/B rd
    ELEMENT_NUMBER  = 38,      // rfA rd
    QPU_NUMBER      = 38,      // rfB rd
    HOST_INT        = 38,      // rfA/B wr
    NOP_R           = 39,      // NOP for register read/writes
    VPM_READ        = 48,      // rfA/B rd
    VPM_WRITE       = 48,      // rfA/B wr
    VPMVCD_RD_SETUP = 49,      // rfA wr
    VPMVCD_WR_SETUP = 49,      // rfB wr
    VPM_LD_WAIT     = 50,      // rfA rd
    VPM_ST_WAIT     = 50,      // rfB rd
    VPM_LD_ADDR     = 50,      // rfA wr
    VPM_ST_ADDR     = 50,      // rfB wr
    SFU_RECIP       = 52,      // rfA/B wr
    SFU_RECIPSQRT   = 53,      // "
    SFU_EXP         = 54,      // "
    SFU_LOG         = 55,      // "
    TMU0_S          = 56,      // "
  };

  uint32_t cond_add = 0;       // reused for LI, BR
  uint32_t cond_mul = 0;
  uint32_t waddr_add = NOP_R;  // reused for LI
  uint32_t waddr_mul = NOP_R;

  uint32_t addOp  = 0;
  uint32_t mulOp  = 0;
  uint32_t muxa   = 0;
  uint32_t muxb   = 0;
  uint32_t raddra = NOP_R;
  uint32_t raddrb = 0;

  uint32_t li_imm  = 0;         // Also used as BR target
  uint32_t sema_id = 0;

  void sf(bool val);
  void ws(bool val);
  void rel(bool val);

  void tag(Tag in_tag, bool imm = false);
  uint64_t encode() const;

  static uint32_t encodeDestReg(Reg reg, RegTag* file);
  static uint32_t encodeSrcReg(Reg reg, RegTag file, uint32_t* mux);

private:
  Tag m_tag           = NOP;
  uint32_t m_sig      = 14;   // 0xe0000000
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
