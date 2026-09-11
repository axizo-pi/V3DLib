#include "global.h"
#include "rtweekend.h"
#include "Support/Timer.h"
#include <cassert>

using namespace V3DLib;

/**
 *
 * - Setting main_memory(true) for pure Scalar does not work,
 *   probably because Array items are global.
 *
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

namespace {

RunMode s_run_mode = RunScalar;

double s_aspect_ratio      = 1.0;  // Ratio of image width over height
int    s_image_width       = 100;  // Rendered image width in pixel count
int    s_image_height      = -1;   // Rendered image height
int    s_samples_per_pixel = 10;   // Count of random samples for each pixel

//
// Viewport and defocus stuff
//
point3 pixel00_loc;          // Location of pixel 0, 0
double defocus_angle = 0;  // Variation angle of rays through each pixel
vec3   defocus_disk_u;       // Defocus disk horizontal radius
vec3   defocus_disk_v;       // Defocus disk vertical radius

// Copied over from ViewPort
vec3   pixel_delta_u;        // Offset to pixel to the right
vec3   pixel_delta_v;        // Offset to pixel below
point3 center;               // Camera center

/**
 * @brief Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
 */
vec3 sample_square() {
  return vec3(random_double() - 0.5, random_double() - 0.5, 0);
}


MAYBE_UNUSED vec3 sample_disk(double radius) {
  // Returns a random point in the unit (radius 0.5) disk centered at the origin.
  return radius * random_in_unit_disk();
}


point3 defocus_disk_sample() {
  // Returns a random point in the camera defocus disk.
  auto p = random_in_unit_disk();
  return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
}

} // anon namespace


namespace global {

RunMode run_mode() {
  return s_run_mode;
}


void aspect_ratio(double val) {
  s_aspect_ratio = val;
  s_image_height = -1;
}


int image_width() {
  return s_image_width;
}


void image_width(int val) {
  s_image_width  = val;
  s_image_height = -1;
}


int image_height() {
  if (s_image_height == -1) {
    s_image_height = int(s_image_width / s_aspect_ratio);
    s_image_height = (s_image_height < 1) ? 1 : s_image_height;
  }

  return s_image_height;
}


void samples_per_pixel(int val) {
  s_samples_per_pixel = val;
}

int samples_per_pixel() {
  return s_samples_per_pixel;
}


int num_rays() {
  int size = s_image_width*image_height()*s_samples_per_pixel;
  assert(size > 0);
  return size;
}


void defocus_init(ViewPort const &vp, point3 const &in_center) {
  defocus_angle = 0.6;

  // Copy over, required for ray calculation
  pixel_delta_u = vp.pixel_delta_u;
  pixel_delta_v = vp.pixel_delta_v;
  center = in_center;

  // Calculate the camera defocus disk basis vectors.
  auto defocus_radius = vp.focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
  defocus_disk_u = vp.u * defocus_radius;
  defocus_disk_v = vp.v * defocus_radius;

  // Calculate the location of the upper left pixel.
  auto viewport_upper_left = center - (vp.focus_dist * vp.w) - vp.viewport_u/2 - vp.viewport_v/2;
  pixel00_loc = viewport_upper_left + 0.5 * (vp.pixel_delta_u + vp.pixel_delta_v);
}


ray get_ray(int i, int j) {
  timers.start("get_ray(i, j)");

  // Construct a camera ray originating from the defocus disk and directed at a randomly
  // sampled point around the pixel location i, j.

  auto offset = sample_square();
  auto pixel_sample = pixel00_loc
    + ((i + offset.x()) * pixel_delta_u)
    + ((j + offset.y()) * pixel_delta_v);

  auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
  auto ray_direction = pixel_sample - ray_origin;

  ray ret(ray_origin, ray_direction);
  timers.stop("get_ray(i, j)");
  return ret;
}

} // namespace global


void ViewPort::init() {
  // Determine viewport dimensions.
  auto theta = degrees_to_radians(vfov);
  auto h = std::tan(theta/2);
  viewport_height = 2 * h * focus_dist;
  viewport_width = viewport_height * (double(global::image_width())/global::image_height());

  // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
  w = unit_vector(lookfrom - lookat);
  u = unit_vector(cross(vup, w));
  v = cross(w, u);

  // Calculate the vectors across the horizontal and down the vertical viewport edges.
  viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
  viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

  // Calculate the horizontal and vertical delta vectors from pixel to pixel.
  pixel_delta_u = viewport_u / global::image_width();
  pixel_delta_v = viewport_v / global::image_height();
}


bool RayIterator::done() const {
  return !(
         (j < s_image_height)
      && (i < s_image_width)
      && (sample < s_samples_per_pixel)
  );
}


void RayIterator::inc() {
  assert(!done());

  if (sample < s_samples_per_pixel) {
    sample++;
  } else {
    if (i < s_image_width) {
      sample = 0;
      i++;
    } else {
      if (j < s_image_height) {
        sample = 0;
        i      = 0;
        j++;
      } else {
        assert(false);
      }
    }
  }
}


bool RayIterator::next(std::function<void(int index, ray r)> f) {
  if (done()) return false;

  int index = (j*s_image_width +  i)*s_samples_per_pixel + sample;
  ray r = global::get_ray(i, j);

  f(index, r);

  return true;
}
