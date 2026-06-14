#include "camera.h"
#include "rtweekend.h"
#include "hittable.h"
#include "material.h"
#include "qpu.h"
#include "Support/Helpers.h"
#include "Support/basics.h"
#include <cassert>


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


void camera::init_rays() {
  for (int j = 0; j < image_height; j++) {
    for (int i = 0; i < image_width; i++) {
      for (int sample = 0; sample < samples_per_pixel; sample++) {
        ray r = get_ray(i, j);

        int index = qpu::set_ray(r, j, i, sample);

        // DEBUG check
        ray r2 = qpu::get_ray(index);
        assert(same(r, r2));
      }
    }
  }
}


void camera::render(const hittable& world) {
  std::string ret;
  ret << "P3\n" << image_width << " " << image_height << "\n255\n";

  int num_indexes = qpu::num_rays();
  for (int index = 0; index < num_indexes; index += samples_per_pixel) {
    if (index % 10000 == 0)
      std::clog << "\rRays remaining: " << (num_indexes - index) << ' ' << std::flush;

    color pixel_color(0,0,0);
    for (int sample = 0; sample < samples_per_pixel; sample++) {
      ray r2 = qpu::get_ray(index + sample);
      pixel_color += ray_color(r2, max_depth, world);
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


color camera::ray_color(const ray& r, int depth, const hittable& world) const {
  // If we've exceeded the ray bounce limit, no more light is gathered.
  if (depth <= 0)
    return color(0,0,0);

  hit_record rec;

  if (world.hit(r, interval(0.001, infinity), rec)) {
    ray scattered;
    color attenuation;
    if (rec.mat->scatter(r, rec, attenuation, scattered))
      return attenuation * ray_color(scattered, depth-1, world);
    return color(0,0,0);
  }

  vec3 unit_direction = unit_vector(r.direction());
  auto a = 0.5*(unit_direction.y() + 1.0);
  return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}

