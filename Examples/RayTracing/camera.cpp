#include "camera.h"
#include "rtweekend.h"
#include "hittable.h"
#include "material.h"
#include "qpu.h"
#include "global.h"
#include "Support/Helpers.h"
#include "Support/basics.h"
#include <cassert>

using namespace V3DLib;


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
 * The number of samples per ray is determined by `samples_per_pixel`.
 */
void camera::init_rays() {
  for (int j = 0; j < global::image_height(); j++) {
    for (int i = 0; i < global::image_width(); i++) {
      for (int sample = 0; sample < global::samples_per_pixel(); sample++) {
        ray r = global::get_ray(i, j);
        qpu::set_ray(r, j, i, sample);
      }
    }
  }
}


void camera::render(const hittable& world) {
  warn << "Called render()";

  std::string ret;
  ret << "P3\n" << global::image_width() << " " << global::image_height() << "\n255\n";

  int num_indexes = global::num_rays();
  warn << "num_indexes: " << num_indexes;

  int index_limit = (num_indexes > 50000)?10000:(
      (num_indexes > 10000)?1000:(
        (num_indexes > 1000)?100:10
      )
    );

  int cur_limit = 0;

  //
  // QPU Calculation
  //
  if (global::run_mode() != RunScalar) {
    timers.start("QPU run");
    for (int index = 0; index < num_indexes; index++) {
      ray r = qpu::get_ray(index);
      qpu::hittable_list_hit(r, index);
    }
    timers.stop("QPU run");
  }


  for (int index = 0; index < num_indexes; index += global::samples_per_pixel()) {
    if (index >= cur_limit) {
      std::clog << "\rRays remaining: " << (num_indexes - index) << ' ' << std::flush;
      cur_limit += index_limit;
    }

    // Calculate color averaged over samples
    color pixel_color(0,0,0);
    for (int sample = 0; sample < global::samples_per_pixel(); sample++) {
      int index2 = (index + sample);
      ray r2 = qpu::get_ray(index2);

      pixel_color += ray_color(r2, max_depth, world, index2, (global::run_mode() == RunCheck));
    }

    ret << write_color(pixel_samples_scale * pixel_color);
  }

  std::clog << "\rDone.                 \n";
  V3DLib::to_file("out.ppm", ret);
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
/*
      warn << "arr: " << hit_records::dump(ray_index);

      std::string msg;
      msg << "TODO qpu_hit ray_index: " << ray_index;
      assertq(false, msg);
*/
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
    if (do_qpu) {
      if (depth == max_depth) {
        //warn << "rec: " << rec.dump();
        //warn << "arr: " << hit_records::dump(ray_index);
        hit_records::check(ray_index, rec);
      }
    }

    // Scatter is skipped for qpu (for now, I hope)
    ray scattered;
    color attenuation;
    if (rec.mat->scatter(r, rec, attenuation, scattered)) {
      //warn << "Scatter!";
      return attenuation * ray_color(scattered, depth-1, world, ray_index, false);
    } else {
      warn << "Scatter fail";
    }
    return color(0,0,0);
  }

  // Set default color for miss
  vec3 unit_direction = unit_vector(r.direction());
  auto a = 0.5*(unit_direction.y() + 1.0);
  return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}

