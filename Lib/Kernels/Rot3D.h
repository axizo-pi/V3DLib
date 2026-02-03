#ifndef _V3DLIB_KERNELS_ROT3D_H_
#define _V3DLIB_KERNELS_ROT3D_H_
#include "V3DLib.h"

namespace kernels {

using namespace V3DLib;

void scalar_rot3D(int n, float rot_x, float rot_y, float rot_z, float *x, float *y, float *z);
void vector_rot3D(Int n, Float rot_x, Float rot_y, Float rot_z, Float::Ptr x, Float::Ptr y, Float::Ptr z);

}  // namespace kernels

#endif  // _V3DLIB_KERNELS_ROT3D_H_
