#ifndef V3DLIB_ROT3D_KERNEL_H
#define V3DLIB_ROT3D_KERNEL_H
#include "Kernels/Rot3D.h"

using KernelType = decltype(kernels::vector_rot3D);  // All kernel functions except scalar have same prototype

void run_qpu_kernel(KernelType &kernel);

#endif //  V3DLIB_ROT3D_KERNEL_H
