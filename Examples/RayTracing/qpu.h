#ifndef _RAYTRACING_QPU_H
#define _RAYTRACING_QPU_H
#include "ray.h"
#include "sphere.h"
#include "global/log.h"

namespace qpu {

void kernels_init();
void init_arrays(int image_width, int image_height, int samples_per_pixel, int num_spheres);

int num_rays();
int set_ray(ray const &in_ray, int r, int c, int spp);
ray get_ray(uint32_t index);

int  num_spheres();
void add_sphere(int index, sphere const &in_sphere);
sphere get_sphere(int index);
bool same_sphere(int index, sphere const &s);

void hittable_list_hit(const ray &r);
bool check_ret(int sphere_index, vec3 const &v);
bool check_f(int sphere_index, double val, float precision = 0, int bit_min = 0);
bool check_sign(int sphere_index, double val);
void end();

}  // namespace qpu

bool same(ray const &lhs, ray const &rhs);

#endif // _RAYTRACING_QPU_H
