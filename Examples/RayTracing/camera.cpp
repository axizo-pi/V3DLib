#include "camera.h"
#include "rtweekend.h"
#include "hittable.h"
#include "material.h"
#include "qpu.h"
#include "Support/Helpers.h"
#include "Support/basics.h"
#include <cassert>

using namespace V3DLib;

/**
 * @brief Indicator for run mode.
 *
 * There are three options for running:
 *
 * - RunQPU    - Run QPU calculation only
 * - RunScalar - Run CPU calculation only
 * - RunCheck  - Run both QPU and CPU and compare output where checks are enabled
 */
enum RunMode {
	RunQPU,
	RunScalar,
	RunCheck
};

const RunMode run_mode = RunScalar;

/**
 * | RunMode | Width | Run  Time (s) | Comment                   |
 * |---------|-------|---------------|---------------------------|
 * | Scalar  |  64   |  2.147673     |                           |
 * | QPU     |  64   |  3.962608     | Kernel call per every ray |
 * | Scalar  | 128   |  8.599195     |                           |
 * | QPU     | 128   | 16.356966     | 1 call/ray                |
 * | Scalar  | 192   |               | heap overflow             |
 * | QPU     | 192   |               | 1 call/ray, heap overflow |
 * | QPU     | 256   |               | 1 call/ray, heap overflow |
 */


void camera::initialize() {
  image_height = int(image_width / aspect_ratio);
  image_height = (image_height < 1) ? 1 : image_height;

  pixel_samples_scale = 1.0 / samples_per_pixel;

  center = lookfrom;

  // Determine viewport dimensions.
  auto theta = degrees_to_radians(vfov);
  auto h = std::tan(theta/2);
  auto viewport_height = 2 * h * focus_dist;
  auto viewport_width = viewport_height * (double(image_width)/image_height);

  // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
  w = unit_vector(lookfrom - lookat);
  u = unit_vector(cross(vup, w));
  v = cross(w, u);

  // Calculate the vectors across the horizontal and down the vertical viewport edges.
  vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
  vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

  // Calculate the horizontal and vertical delta vectors from pixel to pixel.
  pixel_delta_u = viewport_u / image_width;
  pixel_delta_v = viewport_v / image_height;

  // Calculate the location of the upper left pixel.
  auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
  pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

  // Calculate the camera defocus disk basis vectors.
  auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
  defocus_disk_u = u * defocus_radius;
  defocus_disk_v = v * defocus_radius;
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
  for (int j = 0; j < image_height; j++) {
    for (int i = 0; i < image_width; i++) {
      for (int sample = 0; sample < samples_per_pixel; sample++) {
        ray r = get_ray(i, j);
        /* int index = */ qpu::set_ray(r, j, i, sample);

        // OK, check confirmed exact
        //ray r2 = qpu::get_ray(index);
        //assert(same(r, r2));
      }
    }
  }
}


void camera::render(const hittable& world) {
  warn << "Called render()";

  std::string ret;
  ret << "P3\n" << image_width << " " << image_height << "\n255\n";

  int num_indexes = qpu::num_rays();
  warn << "num_indexes: " << num_indexes;

  int index_limit = (num_indexes < 10000)?(num_indexes < 1000? 10: 100): 1000;
  int cur_limit = 0;

	//
	// QPU Calculation
	//
	if (run_mode != RunScalar) {
	  timers.start("QPU run");
  	for (int index = 0; index < num_indexes; index++) {
	    ray r = qpu::get_ray(index);
	    qpu::hittable_list_hit(r, index);
		}
	  timers.stop("QPU run");
	}


  for (int index = 0; index < num_indexes; index+= samples_per_pixel) {
    if (index >= cur_limit) {
      std::clog << "\rRays remaining: " << (num_indexes - index) << ' ' << std::flush;
      cur_limit += index_limit;
    }

		// Calculate color averaged over samples
    color pixel_color(0,0,0);
    for (int sample = 0; sample < samples_per_pixel; sample++) {
      int index2 = (index + sample);
      ray r2 = qpu::get_ray(index2);

      pixel_color += ray_color(r2, max_depth, world, index2, (run_mode == RunCheck));
    }

    ret << write_color(pixel_samples_scale * pixel_color);
  }

  std::clog << "\rDone.                 \n";
  V3DLib::to_file("out.ppm", ret);
}


ray camera::get_ray(int i, int j) const {
  // Construct a camera ray originating from the defocus disk and directed at a randomly
  // sampled point around the pixel location i, j.

  auto offset = sample_square();
  auto pixel_sample = pixel00_loc
    + ((i + offset.x()) * pixel_delta_u)
    + ((j + offset.y()) * pixel_delta_v);

  auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
  auto ray_direction = pixel_sample - ray_origin;

  return ray(ray_origin, ray_direction);
}


vec3 camera::sample_square() const {
  // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
  return vec3(random_double() - 0.5, random_double() - 0.5, 0);
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

  if (do_hit(run_mode == RunQPU)) {
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

