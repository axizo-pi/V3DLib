#include "qpu.h"
#include "kernel.h"
#include "V3DLib.h"
#include "Support/Helpers.h"  // resize_16()
#include <cmath>

using namespace std;
using namespace V3DLib;

namespace qpu {
namespace {

int s_exact_match   = 0;;
int	s_total_matches = 0;

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


	std::string dump_vecs(int num_elems) const {
		std::string ret;

		for (int i = 0; i < num_elems; ++i) {
			ret << "\n  " << dump_vec(i);
		}

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

// DEBUG
points       ret;
Float::Array ret_f;


bool same_vec(int index, vec3 const &v, points const &pts, float precision = 0.0f) {
	const int bit_min = 2; //5;

	int bits_x = bit_diff(pts.x[index], (float) v.x(), bit_min);
	int bits_y = bit_diff(pts.y[index], (float) v.y(), bit_min);
	int bits_z = bit_diff(pts.z[index], (float) v.z(), bit_min);

	bool ret_x = (bits_x == -1);
	bool ret_y = (bits_y == -1);
	bool ret_z = (bits_z == -1);

	bool ret = ret_x && ret_y && ret_z;

	if (ret) return true;  // Assume all is well

	//warn << "same_vec failed for index: " << index << "\n"
	//	   << "bits: (" << bits_x << ", " << bits_y << ", " << bits_z << ")";

	//
	// There is a major discrepancy between bit check and range check.
	// When eyeballing it, I see no difference.
	//
	// My hypothesis is that fabs() is not consistent, which I can hardly believe.
	// For the time being, doing both checks (bits and range).
	//

	//float length = (float) v.length();

	float diff_x = fabs(pts.x[index] - ((float) v.x())); ///length;
	float diff_y = fabs(pts.y[index] - ((float) v.y())); ///length;
	float diff_z = fabs(pts.z[index] - ((float) v.z())); ///length;

	if (diff_x == 0) s_exact_match++;
	if (diff_y == 0) s_exact_match++;
	if (diff_z == 0) s_exact_match++;
	s_total_matches += 3;

	ret_x  = ret_x || (diff_x <= precision);
	ret_y  = ret_y || (diff_y <= precision);
	ret_z  = ret_z || (diff_z <= precision);

	ret =  ret_x && ret_y && ret_z;

	if (!ret) {
		//breakpoint; 

		warn << "same_vec failed for index: " << index << "\n"
			   << "bits: (" << bits_x << ", " << bits_y << ", " << bits_z << ")\n"
			   << "ret : (" << ret_x << ", " << ret_y << ", " << ret_z << ")\n"
			   << "   v:    " << v.dump(true) << "\n"
			   << " pts: "    << pts.dump_vec(index) << "\n"
				 << "diff:         (" << diff_x << ", " << diff_y << ", " << diff_z << ")";
	}

	return ret;
}


bool same_float(int index, float val, Float::Array &ret_f, float precision = 0.0f, int bit_min = 0) {
	assert(bit_min >= -1);

	int bits = bit_diff(ret_f[index], val, bit_min);

	bool ret = (bits == -1);


	//float diff = fabs(ret_f[index] - val);
	float diff = abs(val - ret_f[index])/val;

	if (diff == 0) s_exact_match++;
	s_total_matches += 1;

	if (ret) return true;  // Assume all is well

	//warn << "same_float failed for index: " << index << "\n"
	//	   << "bits: (" << bits<< ")";

	ret  = ret || (diff <= precision);

	if (!ret) {
		warn << "same_float failed for index: " << index << "\n"
			   << "bits    : (" << bits << ")\n"
			   << "ret     : (" << ret << ")\n"
			   << "val     : " << val << "\n"
			   << "ret_f[" << index << "]: " << ret_f[index] << "\n"
				 << "diff    : " << diff;
	}

	return ret;
}

}  // anon namespace

void kernels_init() {
	kernel::init();
}

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

  ret.alloc(num_spheres);
  ret_f.alloc(num_spheres);
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
 * Disgustingly inefficient, like 20x worse than original access.
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

	if (diff_x == 0) s_exact_match++;
	if (diff_y == 0) s_exact_match++;
	if (diff_z == 0) s_exact_match++;
	if (diff_r == 0) s_exact_match++;
	s_total_matches += 4;

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


void hittable_list_hit(const ray &r) {
	//static bool did_first = false;
	timers.start("hittable_list_hit");
  assert(s_num_spheres > 0);
  int N_spheres = resize_16(s_num_spheres);
/*
	if (!did_first) {
		warn << "Pre:"
	  	   //<< "  " << ret.dump_vecs(2)
			   << "\n  "
				 << r.direction().dump()
			   << "\n  "
			   << ret_f[0] << ", " << ret_f[1];
		;
	}
*/
	kernel::sphere_hit(r, N_spheres, center.x, center.y, center.z, radius, ret.x, ret.y, ret.z, ret_f);

	timers.stop("hittable_list_hit");
/*
	if (!did_first) {
		warn << "Post:"
			   //<< "  " << ret.dump_vecs(2)
			   << "\n  "
			   << ret_f[0] << ", " << ret_f[1];
		;
	}
	did_first = true;
*/	
}


bool check_ret(int sphere_index, vec3 const &v) {
	timers.start("check_ret");

	// Most comparisons are exact
  bool ret = same_vec(sphere_index, v, qpu::ret, 1.0e-6f);

	timers.stop("check_ret");
	return ret;
}


bool check_f(int sphere_index, double val, float precision, int bit_min) {
	timers.start("check_f");
  bool ret = same_float(sphere_index, (float) val, qpu::ret_f, precision, bit_min);
	timers.stop("check_f");
	return ret;
}


bool check_sign(int sphere_index, double val) {
	int sign_s = (ret_f[sphere_index] < 0)?-1:1;
	int sign_v = (val < 0)?-1:1;
	bool ret = (sign_s == sign_v);

	if (!ret) {
		warn << "check_sign fail, sphere: " << ret_f[sphere_index] << ", val: " << val;
	}

	return ret;
}


void end() {
	warn << "exact matches: " << s_exact_match << " out of " << s_total_matches
		   << ", " << (int) (100.0*s_exact_match/s_total_matches) << "%";
}

}  // namespace qpu


bool same(ray const &lhs, ray const &rhs) {
  auto diff = lhs.abs_diff(rhs);

  double precision = 1.0e-7f;

  return diff.origin().near_zero(precision) && diff.direction().near_zero(precision);
}
