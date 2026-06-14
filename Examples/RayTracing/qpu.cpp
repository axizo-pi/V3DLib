#include "qpu.h"
#include "V3DLib.h"
#include "Support/Helpers.h"  // resize_16()
#include <cmath>

using namespace std;
using namespace V3DLib;

namespace qpu {
namespace {

int image_width       = 0;
int image_height      = 0;
int samples_per_pixel = 0;
int s_num_spheres     = 0;

struct points {
  void alloc(int size) {
    x.alloc(size);
    y.alloc(size);
    z.alloc(size);

    x.fill(0.0f);
    y.fill(0.0f);
    z.fill(0.0f);
  }


  void set_vec(int index, vec3 const &vec) {
    x[index] = (float) vec.x();
    y[index] = (float) vec.y();
    z[index] = (float) vec.z();
  }


	std::string dump_vec(int index) const {
		std::string ret;

		ret << "vec[" << index << "]: ("
    		<< x[index] << ", "
    		<< y[index] << ", "
    		<< z[index] << ")";

		return ret;
  }


  vec3 to_vec(int index) {
    vec3 ret((double) x[index], (double) y[index], (double) z[index]);
    return ret;
  }

  Float::Array x;
  Float::Array y;
  Float::Array z;
};  


// Ray coordinates
points origin;
points direction;

// Sphere coordinates
points       center;
Float::Array radius;

}  // anon namespace


void init_arrays(int image_width, int image_height, int samples_per_pixel, int num_spheres) {
  qpu::image_width       = image_width;
  qpu::image_height      = image_height;
  qpu::samples_per_pixel = samples_per_pixel;

  uint32_t size = image_width*image_height*samples_per_pixel;
  assert(size % 16 == 0);

  origin.alloc(size);
  direction.alloc(size);

  num_spheres = resize_16(num_spheres);
  s_num_spheres = num_spheres;
  assert(num_spheres % 16 == 0);
  warn << "num_spheres: " << num_spheres;
  center.alloc(num_spheres);
  radius.alloc(num_spheres);

}


int num_rays() {
  int size = image_width*image_height*samples_per_pixel;
  assert(size > 0);
  return size;
}


/**
 *
 * @param  r    Row index of ray
 * @param  c    column index of ray
 * @param  spp  Samples per pixel
 * @return      Index into Float arrays
 */
int set_ray(ray const &in_ray, int r, int c, int spp) {
  int index = (r*image_width +  c)*samples_per_pixel + spp;
  //warn << "set_ray index: " << index;

  assert(origin.x[index] == 0.0f);  // Assuming rest of arrays also zero

  origin.set_vec(index, in_ray.origin());
  direction.set_vec(index, in_ray.direction());

  return index;
}


ray get_ray(uint32_t index) {
  vec3 tmp_origin    = origin.to_vec(index);
  vec3 tmp_direction = direction.to_vec(index);

  return ray(tmp_origin, tmp_direction);
}


int num_spheres() {
  assert(s_num_spheres > 0);
  return s_num_spheres;
}


void add_sphere(int index, sphere const &in_sphere) {
	assert(0 <= index && index < num_spheres());

	center.set_vec(index, in_sphere.center());
	radius[index] = (float) in_sphere.radius();
}


/**
 * Disgustingly inefficient, like 20x worse.
 * TODO: set up things to avoid having to use it.
 *
 * Material not added here
 */
sphere get_sphere(int index) {
	assert(0 <= index && index < num_spheres());
	return sphere(center.to_vec(index), (double) radius[index], nullptr);
}


bool same_sphere(int index, sphere const &s) {
	// Amazingly, comparison is exact
	float Precision = 0; //1.0e-7f;

	assert(0 <= index && index < num_spheres());
	//warn << center.dump_vec(index);

	auto const &rhs = s.center();
	float length = (float) rhs.length();

	float diff_x = fabs(center.x[index] - (float) rhs.x())/length;
	float diff_y = fabs(center.y[index] - (float) rhs.y())/length;
	float diff_z = fabs(center.z[index] - (float) rhs.z())/length;
	float diff_r = fabs(radius[index]   - (float) s.radius())/((float) s.radius());

	//warn << "same_sphere diff_x: " << diff_x;
	//warn << "same_sphere diff_y: " << diff_y;
	//warn << "same_sphere diff_z: " << diff_z;

	return 
		diff_x <= Precision &&
		diff_y <= Precision &&
		diff_z <= Precision &&
		diff_r <= Precision
	;
}

}  // namespace qpu


bool same(ray const &lhs, ray const &rhs) {
  auto diff = lhs.abs_diff(rhs);

  double precision = 1.0e-7f;

  return diff.origin().near_zero(precision) && diff.direction().near_zero(precision);
}
