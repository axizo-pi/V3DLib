#ifndef _RAYTRACING_KERNEL_H
#define _RAYTRACING_KERNEL_H
#include "ray.h"
#include "V3DLib.h"

namespace kernel {

void init();

void sphere_hit(
  ray const &r, int N_spheres,
  V3DLib::Float::Array &in_center_x, V3DLib::Float::Array &in_center_y, V3DLib::Float::Array &in_center_z,
  V3DLib::Float::Array &in_radius,
  V3DLib::Float::Array &ret_x, V3DLib::Float::Array &ret_y, V3DLib::Float::Array &ret_z,
  V3DLib::Float::Array &ret_f,
  V3DLib::Int::Array   &ret_valid
);

} // namespace kernel

#endif // _RAYTRACING_KERNEL_H
