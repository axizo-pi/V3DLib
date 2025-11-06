///////////////////////////////////////////////////////////////////////////////
//
// Interface for the broadcomm v3d calls and definitions
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _V3D_INSTR_DUMP_H
#define _V3D_INSTR_DUMP_H
#include "broadcom/qpu/qpu_instr.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct v3d_device_info const *devinfo();  // defined in v3d/v3d.cpp
uint8_t devinfo_ver();
void instr_dump(char *buffer, struct v3d_qpu_instr *instr);
bool instr_unpack(uint64_t packed_instr, struct v3d_qpu_instr *instr);
uint64_t instr_pack(struct v3d_qpu_instr const *instr);
const char *instr_mnemonic(const struct v3d_qpu_instr *instr);
bool small_imm_pack(uint32_t value, uint32_t *packed_small_immediate);

const char *qpu_decode(struct v3d_qpu_instr const *instr);
const char *qpu_disasm(uint64_t packed);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif  // _V3D_INSTR_DUMP_H
