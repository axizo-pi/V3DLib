#include "v3d_api.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "broadcom/qpu/qpu_disasm.h"
#include "util/ralloc.h"  // ralloc_free()


static const struct v3d_qpu_alu_instr ALU_NOP = {
    add: {
      op: V3D_QPU_A_NOP,
      waddr: 6,
      magic_write: true,
      output_pack: V3D_QPU_PACK_NONE,

      a: {
        mux: V3D_QPU_MUX_R0,
        unpack: V3D_QPU_UNPACK_NONE 
      },
      b: {
        mux: V3D_QPU_MUX_R0,
        unpack: V3D_QPU_UNPACK_NONE
      }
    },
    mul: {
      op: V3D_QPU_M_NOP,
      waddr: 6,
      magic_write: true,
      output_pack: V3D_QPU_PACK_NONE,

      a: {
        mux: V3D_QPU_MUX_R0,
        unpack: V3D_QPU_UNPACK_NONE, 
      },
      b: {
        mux: V3D_QPU_MUX_R4,
        unpack: V3D_QPU_UNPACK_NONE
      }
    }
};


/**
 * Following functions currently unused. They might
 * have a use later on.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"


static bool isNopAdd( const struct v3d_qpu_alu_instr *src) {
  return (
      src->add.op == ALU_NOP.add.op &&
      src->add.a.mux == ALU_NOP.add.a.mux &&
      src->add.b.mux == ALU_NOP.add.b.mux &&
      src->add.waddr == ALU_NOP.add.waddr &&
      src->add.magic_write == ALU_NOP.add.magic_write &&
      src->add.output_pack == ALU_NOP.add.output_pack &&
      src->add.a.unpack == ALU_NOP.add.a.unpack &&
      src->add.b.unpack == ALU_NOP.add.b.unpack
  );
}


static bool isNopMul( const struct v3d_qpu_alu_instr *src) {
  return (
      src->mul.op == ALU_NOP.mul.op &&
      src->mul.a.mux == ALU_NOP.mul.a.mux &&
      src->mul.b.mux == ALU_NOP.mul.b.mux &&
      src->mul.waddr == ALU_NOP.mul.waddr &&
      src->mul.magic_write == ALU_NOP.mul.magic_write &&
      src->mul.output_pack == ALU_NOP.mul.output_pack &&
      src->mul.a.unpack == ALU_NOP.mul.a.unpack &&
      src->mul.b.unpack == ALU_NOP.mul.b.unpack
  );
}

#define CASE(l)  case V3D_QPU_##l: ret = #l; break;

static const char *dump_cond(enum v3d_qpu_cond val) {
  char *ret = "<<UNKNOWN>>";

  switch (val) {
    CASE(COND_NONE)
    CASE(COND_IFA)
    CASE(COND_IFB)
    CASE(COND_IFNA)
    CASE(COND_IFNB)
  }

  assert(ret != 0);
  return ret;
}


static const char *dump_pf(enum v3d_qpu_pf val) {
  char *ret = "<<UNKNOWN>>";

  switch (val) {
    CASE(PF_NONE)
    CASE(PF_PUSHZ)
    CASE(PF_PUSHN)
    CASE(PF_PUSHC)
  }

  assert(ret != 0);
  return ret;
}


static const char *dump_uf(enum v3d_qpu_uf val) {
  char *ret = "<<UNKNOWN>>";

  switch (val) {
    CASE(UF_NONE)
    CASE(UF_ANDZ)
    CASE(UF_ANDNZ)
    CASE(UF_NORNZ)
    CASE(UF_NORZ)
    CASE(UF_ANDN)
    CASE(UF_ANDNN)
    CASE(UF_NORNN)
    CASE(UF_NORN)
    CASE(UF_ANDC)
    CASE(UF_ANDNC)
    CASE(UF_NORNC)
    CASE(UF_NORC)
  }

  assert(ret != 0);
  return ret;
}


static const char *dump_msfign(enum v3d_qpu_msfign val) {
  char *ret = "<<UNKNOWN>>";

  switch (val) {
    CASE(MSFIGN_NONE)
    CASE(MSFIGN_P)
    CASE(MSFIGN_Q)
  }

  assert(ret != 0);
  return ret;
}


static const char *dump_branch_dest(enum v3d_qpu_branch_dest val) {
  char *ret = "<<UNKNOWN>>";

  switch (val) {
    CASE(BRANCH_DEST_ABS)
    CASE(BRANCH_DEST_REL)
    CASE(BRANCH_DEST_LINK_REG)
    CASE(BRANCH_DEST_REGFILE)
  }

  assert(ret != 0);
  return ret;
}




static const char *dump_mux(struct v3d_qpu_input input) {
  enum v3d_qpu_mux val = input.mux;
  if (devinfo_ver() >= 71) {
    return "<<No mux on vc7>>";
  }

  char *ret = "<<UNKNOWN>>";

  switch (val) {
    CASE(MUX_R0)
    CASE(MUX_R1)
    CASE(MUX_R2)
    CASE(MUX_R3)
    CASE(MUX_R4)
    CASE(MUX_R5)
    CASE(MUX_A)
    CASE(MUX_B)
  }

  assert(ret != 0);
  return ret;
}


static const char *dump_output_pack(enum v3d_qpu_output_pack val) {
  static char buffer[64];
  char *ret = "<<UNKNOWN>>";

  switch (val) {
    CASE(PACK_NONE)
    CASE(PACK_L)
    CASE(PACK_H)
    default:
      sprintf(buffer, "<<UNKNOWN>> (%u)", (unsigned) val);
      ret = buffer;
      break;
  }

  assert(ret != 0);
  return ret;
}


static const char *dump_input_unpack(enum v3d_qpu_input_unpack val) {
//  printf("v3d_qpu_input_unpack: %u\n", val);

  static char buffer[64];
  char *ret = "<<UNKNOWN>>";

  switch (val) {
    CASE(UNPACK_NONE)
    CASE(UNPACK_ABS)
    CASE(UNPACK_L)
    CASE(UNPACK_H)
    CASE(UNPACK_REPLICATE_32F_16)
    CASE(UNPACK_REPLICATE_L_16)
    CASE(UNPACK_REPLICATE_H_16)
    CASE(UNPACK_SWAP_16)
    default:
      sprintf(buffer, "<<UNKNOWN>> (%u)", (unsigned) val);
      ret = buffer;
      break;
  }

  assert(ret != 0);
  return ret;
}

#undef CASE

#pragma GCC diagnostic pop


///////////////////////////////////////////////////////////////////////////////
// Wrappers for the actual calls we want to make.
// 
// These are here to beat the external linkage issues I've been having.
///////////////////////////////////////////////////////////////////////////////

bool instr_unpack(uint64_t packed_instr, struct v3d_qpu_instr *instr) {
  return v3d_qpu_instr_unpack(devinfo(), packed_instr, instr);
}


uint64_t instr_pack(struct v3d_qpu_instr const *instr) {
  uint64_t packed_instr;
  v3d_qpu_instr_pack(devinfo(), instr, &packed_instr);
  return packed_instr;
}


const char *instr_mnemonic(const struct v3d_qpu_instr *instr) {
  static char buffer[256];

  const char *decoded = v3d_qpu_decode(devinfo(), instr);
  snprintf(buffer, 256, decoded);
  ralloc_free((char *)decoded);

  return buffer;
}


bool small_imm_pack(uint32_t value, uint32_t *packed_small_immediate) {
  return v3d_qpu_small_imm_pack(devinfo(), value, packed_small_immediate);
}


const char *qpu_decode(struct v3d_qpu_instr const *instr) {
  return v3d_qpu_decode(devinfo(), instr);
}


const char *qpu_disasm(uint64_t packed) {
  return v3d_qpu_disasm(devinfo(), packed);
}
