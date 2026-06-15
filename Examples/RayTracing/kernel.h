#ifndef _RAYTRACING_KERNEL_H
#define _RAYTRACING_KERNEL_H
#include "ray.h"
#include "V3DLib.h"

namespace kernel {

void init();

void sphere_hit(
	ray const &r, int N_spheres,
	V3DLib::Float::Array &center_x, V3DLib::Float::Array &center_y, V3DLib::Float::Array &center_z,
	V3DLib::Float::Array &radius,
	V3DLib::Float::Array &ret_x, V3DLib::Float::Array &ret_y, V3DLib::Float::Array &ret_z,
	V3DLib::Float::Array &ret_f
);

} // namespace kernel

#endif // _RAYTRACING_KERNEL_H
