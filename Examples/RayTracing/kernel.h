#ifndef _RAYTRACING_KERNEL_H
#define _RAYTRACING_KERNEL_H
#include "ray.h"
#include "V3DLib.h"

namespace kernel {

using namespace V3DLib;

void init();

void sphere_hit(
  int ray_index,
  int num_rays,
  Float::Array &in_origin_x, Float::Array &in_origin_y, Float::Array &in_origin_z,
  Float::Array &in_direction_x, Float::Array &in_direction_y, Float::Array &in_direction_z,
  int N_spheres,
  Float::Array &in_center_x, Float::Array &in_center_y, Float::Array &in_center_z,
  Float::Array &in_radius,
  Float::Array &rec_p_x, Float::Array &rec_p_y, Float::Array &rec_p_z,
  Float::Array &rec_normal_x, Float::Array &rec_normal_y, Float::Array &rec_normal_z,
  Float::Array &rec_t,
  Float::Array &rec_front_face,
  Int::Array   &rec_sphere_index
);

} // namespace kernel

#endif // _RAYTRACING_KERNEL_H
