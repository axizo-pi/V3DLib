#include "camera.h"
#include "qpu.h"
#include "material.h"
#include "Support/Timer.h"

using namespace V3DLib;

namespace {

color def_color1(1.0, 1.0, 1.0);
color def_color2(0.5, 0.7, 1.0);

/**
 * @brief Determine default color for miss
*/
color default_color(ray const &r) {
  vec3 unit_direction = unit_vector(r.direction());
  auto a = 0.5*(unit_direction.y() + 1.0);
  return (1.0 - a)*def_color1 + a*def_color2;
}

} // anon namspace

void camera::initialize() {
  pixel_samples_scale = 1.0 / global::samples_per_pixel();
  center = lookfrom;

  ViewPort::init();
  global::defocus_init(*this, center);
}


/**
 * @brief Initialize qpu rays
 *
 * The returned rays from `get_ray()` have a small offset around the actual coordinates.
 * For qpu, a number of these 'diffused' rays are passed in for the calculation.
 *
 * Timing inconsequential.
 *
 * @return number of rays added. This is zero when done.
 */
int camera::init_rays() {
  static int call_count = 0;
  call_count++;

  rays::reset();

  while (ray_iterator.next([] (int index, ray r) -> bool {
    return rays::set(r, index);
  }));

  int ret = rays::num();
  assert(ret % global::samples_per_pixel() == 0); // Samples per pixel must be in same buffer
  
  return ret;
}


void camera::render(const hittable& world, PPM &ret) {
  timers.start("render");

  int num_indexes = global::num_rays();

  int index_limit = (num_indexes > 1000000)?100000:(
    (num_indexes > 50000)?10000:(
      (num_indexes > 10000)?1000:(
        (num_indexes > 1000)?100:10
      )
    )
  );

  int cur_limit = 0;

  for (int index = rays::first_index(); index < rays::last_index(); index += global::samples_per_pixel()) {
    if (index >= cur_limit) {
      std::clog << "\rRays remaining: " << (num_indexes - index) << ' ' << std::flush;
      cur_limit += index_limit;
    }

    // Calculate color averaged over samples
    color pixel_color(0,0,0);
    for (int sample = 0; sample < global::samples_per_pixel(); sample++) {
      int index2 = (index + sample);
      ray r2 = rays::get(index2);

      pixel_color += ray_color(r2, max_depth, world, index2, (global::run_mode() == RunCheck));
    }

    ret.write_color(pixel_samples_scale * pixel_color);
  }

  timers.stop("render");
}


/**
 * This method is called recursively for scattered rays.
 *
 * @param do_qpu If true, do any activated validation tests between QPU and scalar calculations.
 *               This is done on the first level of hit-calculations, not for scattered rays.
 */
color camera::ray_color(const ray& r, int depth, const hittable& world, int ray_index, bool do_qpu) const {
  // If we've exceeded the ray bounce limit, no more light is gathered.
  if (depth <= 0)
    return color(0,0,0);

  hit_record rec;

  auto do_hit = [&world, &r, ray_index, do_qpu, &rec, depth, this] (bool qpu_hit) -> bool {
    if (qpu_hit && depth == this->max_depth) {
      if (!hit_records::valid(ray_index)) return false;

      rec = hit_records::get(ray_index);
      return true;
    } else {
      bool ret = world.hit(r, interval(0.001, infinity), rec, ray_index, -1, do_qpu);

      if (do_qpu && depth == this->max_depth) {
        assert(hit_records::valid(ray_index) == ret);
      }

      return ret;
    }
  };

  if (do_hit(global::run_mode() == RunQPU)) {
    if (do_qpu && (depth == max_depth)) {
      hit_records::check(ray_index, rec);
    }

    ray   scattered;
    color attenuation;

    // Timing scatter inconsequential
    bool success = rec.mat->scatter(r, rec, attenuation, scattered);
    if (success) {
      return attenuation * ray_color(scattered, depth-1, world, ray_index, false);
    }

    return color(0,0,0);
  }

  return default_color(r);  // Set default color for miss
}
