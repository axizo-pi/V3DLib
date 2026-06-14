#include "hittable_list.h"
#include "Support/Timer.h"
#include "qpu.h"

using namespace V3DLib;

bool hittable_list::hit(const ray& r, interval ray_t, hit_record& rec) const {
  hit_record temp_rec;
  bool hit_anything = false;
  auto closest_so_far = ray_t.max;

	for (int i = 0; i < (int) objects.size(); ++i) {
    sphere const &s1 = (sphere const &) *objects[i];

    if (s1.hit(r, interval(ray_t.min, closest_so_far), temp_rec)) {
      hit_anything = true;
      closest_so_far = temp_rec.t;
      rec = temp_rec;
    }
  }

  return hit_anything;
}
