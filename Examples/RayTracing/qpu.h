#ifndef _RAYTRACING_QPU_H
#define _RAYTRACING_QPU_H
#include "ray.h"

namespace qpu {

void init_arrays(int image_width, int image_height, int samples_per_pixel);
int num_rays();
int set_ray(ray const &in_ray, int r, int c, int spp);
ray get_ray(int index);

}  // namespace qpu

bool same(ray const &lhs, ray const &rhs);

#endif // _RAYTRACING_QPU_H
