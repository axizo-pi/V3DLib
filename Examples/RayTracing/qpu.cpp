#include "qpu.h"
#include "kernel.h"
#include "V3DLib.h"
#include "Support/Helpers.h"  // resize_16()
#include <cmath>

using namespace std;
using namespace V3DLib;

namespace qpu {
namespace {

int s_exact_match   = 0;
int  s_total_matches = 0;
int s_zeroes        = 0;
int s_negatives     = 0;

int image_width       = 0;
int image_height      = 0;
int samples_per_pixel = 0;
int s_num_spheres     = 0;

struct points {
  void alloc(int in_size) {
    size = in_size;
    assert(size > 0);

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
    assert(0 <= index && index < size);
    std::string ret;

    ret << "vec[" << index << "]: ("
        << x[index] << ", "
        << y[index] << ", "
        << z[index] << ")";

    return ret;
  }


  std::string dump_vecs() const {
    assert(size > 0);
    std::string ret;

    for (int i = 0; i < size; ++i) {
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

private:  
  int size = -1;
};  


// Ray coordinates
points origin;
points direction;

// Sphere coordinates
points       center;
Float::Array radius;

// DEBUG
points       ret_p;
Float::Array ret_f;
Int::Array   ret_valid;


bool same_vec(int index, vec3 const &v, points const &pts, int bit_min = 0, bool show_log = true) {
  int bits_x = bit_diff(pts.x[index], (float) v.x(), bit_min);
  int bits_y = bit_diff(pts.y[index], (float) v.y(), bit_min);
  int bits_z = bit_diff(pts.z[index], (float) v.z(), bit_min);

  bool ret_x = (bits_x == -1);
  bool ret_y = (bits_y == -1);
  bool ret_z = (bits_z == -1);

  bool ret = ret_x && ret_y && ret_z;

  if (ret) return true;  // Assume all is well

  if (!ret && show_log) {
    warn << "same_vec failed for index: " << index << "\n"
         << "bits: (" << bits_x << ", " << bits_y << ", " << bits_z << ")\n"
         << "ret : (" << ret_x << ", " << ret_y << ", " << ret_z << ")\n"
         << "   v:     " << v.dump(true) << "\n"
         << " pts: "    << pts.dump_vec(index);
  }

  return ret;
}

namespace {

int same_bits(float val1, float val2, int bit_min = 0) {
  assert(bit_min >= -1);

  int bits = bit_diff(val1, val2, bit_min);

  bool success = (bits == -1);
  if (success) {
    s_exact_match++;
  }
  s_total_matches++;

  return bits;
}

} // anon namespace


bool same_vec(vec3 const &lhs, vec3 const &rhs, int bit_min = 0) {
  int bits_x = same_bits((float) lhs.x(), (float) rhs.x(), bit_min);
  int bits_y = same_bits((float) lhs.y(), (float) rhs.y(), bit_min);
  int bits_z = same_bits((float) lhs.z(), (float) rhs.z(), bit_min);

  bool ret_x = (bits_x == -1);
  bool ret_y = (bits_y == -1);
  bool ret_z = (bits_z == -1);

  bool ret = ret_x && ret_y && ret_z;

  if (!ret) {
    warn << "same_vec() fail bits: (" << bits_x << ", " << bits_y << ", " << bits_z << ")";
  }

  return ret;
}


bool same_float(int index, float val, Float::Array &ret_f, int bit_min = 0) {
  assert(bit_min >= -1);

  int bits = same_bits(ret_f[index] , val, bit_min);

  bool ret = (bits == -1);

  if (!ret) {
    warn << "same_float failed for index: " << index << "\n"
         << "bits    : (" << bits << ")\n"
         << "ret     : (" << ret << ")\n"
         << "val     : " << val << "\n"
         << "ret_f[" << index << "]: " << ret_f[index];
  }

  return ret;
}

}  // anon namespace

std::string origin_dump(int index) {
  return origin.dump_vec(index);
}

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

  s_num_spheres = resize_16(num_spheres);
  assert(s_num_spheres % 16 == 0);
  //warn << "s_num_spheres: " << s_num_spheres;

  center.alloc(s_num_spheres);
  radius.alloc(s_num_spheres);

  ret_p.alloc(s_num_spheres);
  ret_f.alloc(s_num_spheres);
  ret_valid.alloc(s_num_spheres);
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


float get_f(int index) {
  return ret_f[index];
}


vec3 get_ret(int index) {
  return ret_p.to_vec(index);
}


int get_valid(int index) {
  return ret_valid[index];
}


int num_spheres() {
  assert(s_num_spheres > 0);
  return s_num_spheres;
}


void add_sphere(int index, sphere const &in_sphere) {
  //warn << "add_sphere index: " << index << ", num spheres: " << num_spheres();
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
  assert(0 <= index && index < num_spheres());

  int bit_min = -1;
  auto const &s0 = get_sphere(index);

  return same_vec(s0.center(), s.center(), bit_min)
      && bit_diff((float) s0.radius(), (float) s.radius(), bit_min);
}


void hittable_list_hit(const ray &r) {
  assert(s_num_spheres > 0);
  //warn << "hittable_list_hit s_num_spheres: " << s_num_spheres;

  timers.start("hittable_list_hit");
  kernel::sphere_hit(
    r, s_num_spheres,
    center.x, center.y, center.z,
    radius,
    ret_p.x, ret_p.y, ret_p.z,
    ret_f,
    ret_valid
  );
  timers.stop("hittable_list_hit");
}


bool check_ret(int sphere_index, vec3 const &v, int bit_min, bool show_log) {
  timers.start("check_ret");
  bool ret = same_vec(sphere_index, v, qpu::ret_p, bit_min, show_log);
  timers.stop("check_ret");
  return ret;
}


bool check_f(int sphere_index, double val, int bit_min) {
  timers.start("check_f");
  bool ret = same_float(sphere_index, (float) val, qpu::ret_f, bit_min);
  timers.stop("check_f");
  return ret;
}


void end() {
  auto percent = [] (int val) -> std::string {
    std::string ret;
    ret << (int) (100.0*val/s_total_matches) << "%";
    return ret;
  };

  warn << "\n"
       << "  Total        : " << s_total_matches << "\n"
       << "  exact matches: " << s_exact_match << ", " << percent(s_exact_match) << "\n"
       << "  zeroes       : " << s_zeroes      << ", " << percent(s_zeroes)      << "\n"
       << "  negatives    : " << s_negatives   << ", " << percent(s_negatives)
  ;
}


void add_zero() { s_zeroes++; }
void add_negative() { s_negatives++; }

}  // namespace qpu


bool same(ray const &lhs, ray const &rhs) {
  return qpu::same_vec(lhs.origin(), rhs.origin(), -1)
      && qpu::same_vec(lhs.direction(), rhs.direction(), -1);
}
