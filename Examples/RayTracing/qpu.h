#ifndef _RAYTRACING_QPU_H
#define _RAYTRACING_QPU_H
#include "ray.h"
#include "sphere.h"

namespace qpu {

void init_arrays(int num_spheres);
void kernels_init();
void run_kernel();
void end();

}  // namespace qpu


namespace spheres {

int num();
void add(int index, sphere const &in_sphere);
sphere get_a(int index);
bool same(int index, sphere const &s);

}  // namespace spheres


namespace rays {

bool set(ray const &in_ray, int ray_index);
ray  get(uint32_t ray_index, bool absolute_index = true);
int  num();
int  first_index();
int  last_index();
void reset();

}  // namespace rays


namespace hit_records {

std::string dump(int index);
void check(int index, hit_record const &rec);
hit_record get(int ray_index);
bool valid(int ray_index);

} // namespace hit_records

#endif // _RAYTRACING_QPU_H
