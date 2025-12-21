#ifndef _V3DLIB_VC4_INSTR_H_
#define _V3DLIB_VC4_INSTR_H_
#include <stdint.h>
#include "Target/instr/Instr.h"

namespace V3DLib {
namespace vc4 {

class Instr {
public:

  enum Encoding {
    NONE, 
    ALU,
    ALU_SMALL_IMM,
    BRANCH_ENC,
    LOAD_IMM,
    LOAD_IMM_ELEM_SIGNED,
    LOAD_IMM_ELEM_UNSIGNED,
    SEMAPHORE
  };

  enum Signal {
    SOFTWARE_BREAKPOINT        =  0,  // blocked for the time being
    NO_SIGNAL                  =  1,
    THREAD_SWITCH              =  2,  // (not last)
    PROGRAM_END                =  3,  // (Thread End)
    WAIT_FOR_SCOREBOARD        =  4,  // (stall until this QPU can safely access tile buffer)
    SCOREBOARD_UNLOCK          =  5,
    LAST_THREAD_SWITCH         =  6,
    COVERAGE_LOAD              =  7,  // from Tile Buffer to r4
    COLOR_LOAD                 =  8,  // from Tile Buffer to r4
    COLOR_LOAD_AND_PROGRAM_END =  9,
    LOAD_FROM_TMU0             = 10,  // Load (read) data from TMU0 to r4
    LOAD_FROM_TMU1             = 11,  // Load (read) data from TMU1 to r4
    ALPHA_MASK_LOAD            = 12,  //  from Tile Buffer to r4
    ALU_IMMEDIATE              = 13,  // ALU instruction with raddr_b specifying small immediate
                                      // or vector rotate
    LOAD_IMMEDIATE             = 14,
    BRANCH                     = 15
  };


  enum Registers {
    MUX_A           = 6,
    MUX_B           = 7,

    NUM_RF          = 32,
    UNIFORM_READ    = 32,      // rfA/B rd

    ACC_START       = 33,
    ACC_END         = 37,

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


  enum Unpack {
    NO_UNPACK     = 0, // 32->32 No unpack (NOP)
    UNPACK_16A_32 = 1, // 16a->32 Float16float32 if any ALU consuming data executes float instruction,
                       //  else signed int16->signed int32
    UNPACK_16B_32 = 2, // 16b->32 "
    UNPACK_8D_32R = 3, // 8d->32  Replicate ms byte (alpha) across word: result = {8d, 8d, 8d, 8d}
    UNPACK_8A_32  = 4, // 8a->32  8-bit color value (in range [0, 1.0]) to 32 bit float if
                       // any ALU consuming data executes float instruction, else unsigned int8  int32
    UNPACK_8B_32  = 5, // 8b->32  "
    UNPACK_8C_32  = 6, // 8c->32  "
    UNPACK_8D_32  = 7, // 8d->32  "

    UNPACK_SIZE   = 8
  };

  // Source of conversion is always 32-bits
  enum Pack {
    NO_PACK       =  0,  // (NOP)
    PACK_TO_16A   =  1,  // Convert to 16 bit float if input was float result, else convert
                         // to int16 (no saturation, just take ls 16 bits)
    PACK_TO_16B   =  2,  // "
    PACK_TO_8888  =  3,  // Convert to 8-bit unsigned int (no saturation, just take ls 8 bits)
                         //  and replicate across all bytes of 32-bit word
    PACK_TO_8A    =  4,  // Convert to 8-bit unsigned int (no saturation, just take ls 8 bits)
    PACK_TO_8B    =  5,  // "
    PACK_TO_8C    =  6,  // "
    PACK_TO_8D    =  7,  // "
    PACK_TO_32    =  8,  // Saturate (signed) 32-bit number (given overflow/carry flags)
    PACK_TO_16AS  =  9,  // Convert to 16 bit float if input was float result, else convert
                         // to signed 16 bit integer (with saturation)
    PACK_TO_16BS  = 10,  // "
    PACK_TO_8888R = 11,  // Convert to 8-bit unsigned int (with saturation) and replicate
                         // across all bytes of 32-bit word
    PACK_TO_8AS   = 12,  // Convert to 8-bit unsigned int (with saturation)
    PACK_TO_8BS   = 13,  // "

    PACK_SIZE     = 14,
  };

  enum BranchCondition {
    ALL_Z_FLAGS_SET   =  0, // &{Z[15:0]}  All Z flags set
    ALL_Z_FLAGS_CLEAR =  1, // &{~Z[15:0]} All Z flags clear
    ANY_Z_FLAGS_SET   =  2, // |{Z[15:0]}  Any Z flags set
    ANY_Z_FLAGS_CLEAR =  3, // |{~Z[15:0]} Any Z flags clear
    ALL_N_FLAGS_SET   =  4, // &{N[15:0]}  All N flags set
    ALL_N_FLAGS_CLEAR =  5, // &{~N[15:0]} All N flags clear
    ANY_N_FLAGS_SET   =  6, // |{N[15:0]}  Any N flags set
    ANY_N_FLAGS_CLEAR =  7, // |{~N[15:0]} Any N flags clear
    ALL_C_FLAGS_SET   =  8, // &{C[15:0]}  All C flags set
    ALL_C_FLAGS_CLEAR =  9, // &{~C[15:0]} All C flags clear
    ANY_C_FLAGS_SET   = 10, // |{C[15:0]}  Any C flags set
    ANY_C_FLAGS_CLEAR = 11, // |{~C[15:0]} Any C flags clear
    // 12-14 reserved
    ALWAYS            = 15, // Always execute (unconditional)

    BR_SIZE = 16
  };


  Encoding enc = NONE;
  Signal sig = NO_SIGNAL;

  // Specific for ALU, ALU_IMMEDIATE
  Unpack unpack    = NO_UNPACK;

  bool    pm       = false;
  Pack    pack     = NO_PACK;
  uint8_t cond_add = 0;
  uint8_t cond_mul = 0;

  bool sf   = false;
  bool ws   = false;

  // Specific for branch
  BranchCondition cond_br = ALWAYS;
  bool            rel     = false;
  bool            reg     = false;

  uint8_t waddr_add = NOP_R;
  uint8_t waddr_mul = NOP_R;

  uint8_t op_mul = 0;
  uint8_t op_add = 0;

  uint8_t raddr_a = NOP_R;
  uint8_t raddr_b = NOP_R;  // Doubles as small_int for ALU_IMMEDIATE

  uint8_t add_a = 0;
  uint8_t add_b = 0;
  uint8_t mul_a = 0;
  uint8_t mul_b = 0;

  uint32_t immediate = 0;

  bool     sa        = false;  // increment if false, decrement if true
  uint32_t semaphore = 0;

  void encode(Target::Instr const &instr);
  uint64_t encode() const;
  std::string dump() const;

  static uint8_t encodeDestReg(Reg reg, RegTag* file);
  static uint8_t encodeSrcReg(Reg reg, RegTag file, uint8_t &mux);

private:
  uint32_t high() const;
  uint32_t low() const;
};

}  // namespace vc4
}  // namespace V3DLib

#endif  // _V3DLIB_VC4_INSTR_H_
