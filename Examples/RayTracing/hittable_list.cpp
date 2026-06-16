#include "hittable_list.h"
#include "Support/Timer.h"
#include "qpu.h"
#include <cassert>

using namespace V3DLib;
using namespace Log;

/**
 * Param sphere_index not used, there for the override
 */
bool hittable_list::hit(const ray& r, interval ray_t, hit_record& rec, int sphere_index, bool qpu_check) const {
  hit_record temp_rec;
  bool hit_anything = false;
  auto closest_so_far = ray_t.max;

	timers.start("hittable_list::hit");

	for (int i = 0; i < (int) objects.size(); ++i) {
    sphere const &s1 = (sphere const &) *objects[i];
		//OK assert(qpu::same_sphere(i, s1));
    //sphere s1 = qpu::get_sphere(i);  // No material, seq fault later on
		//warn << "s1: " << s1.dump();

    if (s1.hit(r, interval(ray_t.min, closest_so_far), temp_rec, i, qpu_check)) {
			//warn << "hittable_list Hit!";
      hit_anything = true;
      closest_so_far = temp_rec.t;
      rec = temp_rec;
    }
  }

	timers.stop("hittable_list::hit");

  return hit_anything;
}
