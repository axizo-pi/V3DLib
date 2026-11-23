#ifndef _V3DLIB_DEVICE_INFO_H
#define _V3DLIB_DEVICE_INFO_H
#include "Support/VCType.h"
#include <stdint.h>

#ifdef __cplusplus
#include <string>

std::string v3d_device_info();

// Used in v3d/instr/v3d_api.c, hence the 'extern' brackets
extern "C" {
#endif

struct v3d_device_info const *devinfo();

#ifdef __cplusplus
}
#endif

#endif // _V3DLIB_DEVICE_INFO_H
