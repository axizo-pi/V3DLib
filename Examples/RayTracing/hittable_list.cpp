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
    sphere const &s0 = (sphere const &) *objects[i];
		//OK assert(qpu::same_sphere(i, s0));
    sphere s1 = qpu::get_sphere(i);  // No material, seq fault later on

		// Copy over the material
		s1.mat(s0.mat());

		//if (i == 0) {
		//	warn << "s1 index 0: " << s1.dump();
		//}

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
