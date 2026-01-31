#ifndef V3DLIB_ROT3D_KERNEL_H
#define V3DLIB_ROT3D_KERNEL_H
#include "Kernels/Rot3D.h"

using KernelType = decltype(kernels::rot3D_1);  // All kernel functions except scalar have same prototype

void run_qpu_kernel(KernelType &kernel);
void run_qpu_kernel_3();

#endif //  V3DLIB_ROT3D_KERNEL_H
