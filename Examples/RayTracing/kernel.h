#ifndef _RAYTRACING_KERNEL_H
#define _RAYTRACING_KERNEL_H
#include "ray.h"
#include "V3DLib.h"

namespace kernel {

using namespace V3DLib;

void init();

void sphere_hit(
  ray const &r, int ray_index,
	int N_spheres,
  Float::Array &in_center_x, Float::Array &in_center_y, Float::Array &in_center_z,
  Float::Array &in_radius,
  Float::Array &rec_p_x, Float::Array &rec_p_y, Float::Array &rec_p_z,
  Float::Array &rec_normal_x, Float::Array &rec_normal_y, Float::Array &rec_normal_z,
  Float::Array &rec_t,
  Float::Array &rec_front_face,
  Int::Array   &rec_sphere_index,
  Float::Array &ret_x, Float::Array &ret_y, Float::Array &ret_z,
  Float::Array &ret_f,
  Int::Array   &ret_valid
);

} // namespace kernel

#endif // _RAYTRACING_KERNEL_H
