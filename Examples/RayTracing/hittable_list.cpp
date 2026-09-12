#include "hittable_list.h"
#include "Support/Timer.h"

using namespace V3DLib;

/**
 * **NOTE**: Param sphere_index not used in the calculation, here for the override.
 */
bool hittable_list::hit(const ray& r, interval ray_t, hit_record& rec, int ray_index,  int sphere_index, bool qpu_check) const {
  hit_record temp_rec;
  bool hit_anything = false;
  auto closest_so_far = ray_t.max;

  timers.start("hittable_list::hit");

  for (int i = 0; i < spheres::size(); ++i) {
    sphere const &s0 = spheres::get(i);              // OK, exact  assert(qpu::same_sphere(i, s0));
    //sphere s1 = qpu::get_sphere(i);                // Performance hog!

    if (s0.hit(r, interval(ray_t.min, closest_so_far), temp_rec, ray_index, i, qpu_check)) {
      //warn << "hittable_list Hit!";
      hit_anything = true;
      closest_so_far = temp_rec.t;
      rec = temp_rec;
      rec.mat = s0.mat(); // Copy over the material, seg fault if not added
    }
  }

  timers.stop("hittable_list::hit");

  return hit_anything;
}
