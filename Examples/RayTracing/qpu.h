#ifndef _RAYTRACING_QPU_H
#define _RAYTRACING_QPU_H
#include "ray.h"
#include "sphere.h"
#include "Support/Timer.h"
#include "global/log.h"

namespace qpu {

std::string origin_dump(int index);

void kernels_init();
void init_arrays(int num_spheres);

bool set_ray(ray const &in_ray, int ray_index);
ray get_ray(uint32_t ray_index, bool absolute_index = true);
int num_rays();
int ray_first_index();
int ray_last_index();
void rays_reset();

int  num_spheres();
void add_sphere(int index, sphere const &in_sphere);
sphere get_sphere(int index);
bool same_sphere(int index, sphere const &s);

void hittable_list_hit(const ray &r, int ray_index);
void end();

}  // namespace qpu

bool same(ray const &lhs, ray const &rhs);

namespace hit_records {

std::string dump(int index);
void check(int index, hit_record const &rec);
hit_record get(int ray_index);
bool valid(int ray_index);

} // namespace hit_records

#endif // _RAYTRACING_QPU_H
