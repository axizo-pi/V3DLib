#ifndef _RAYTRACING_QPU_H
#define _RAYTRACING_QPU_H
#include "ray.h"
#include "sphere.h"

namespace qpu {

void init_arrays(int image_width, int image_height, int samples_per_pixel, int num_spheres);

int num_rays();
int set_ray(ray const &in_ray, int r, int c, int spp);
ray get_ray(uint32_t index);

int  num_spheres();
void add_sphere(int index, sphere const &in_sphere);
sphere get_sphere(int index);
bool same_sphere(int index, sphere const &s);

}  // namespace qpu

bool same(ray const &lhs, ray const &rhs);

#endif // _RAYTRACING_QPU_H
