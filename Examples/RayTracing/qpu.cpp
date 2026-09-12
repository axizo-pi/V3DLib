#include "qpu.h"
#include "global.h"
#include "kernel.h"
#include "Support/Helpers.h"  // resize_16()
#include "Support/dump.h"     // bitdiff_stats()
#include "Support/Timer.h"
#include <limits>             // infinity

using namespace V3DLib;

namespace qpu {
namespace {

// Size of point arrays.
// Value is a decent heuristic, which fits into the default heap size.
const int ArraySize = 92160;

int s_exact_match   = 0;
int s_total_matches = 0;
int s_num_spheres   = 0;


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

    ret //<< "vec[" << index << "]: ("
        << "vec3("
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


struct HitRecords {
  void alloc(int in_size) {
    p.alloc(in_size);
    normal.alloc(in_size);
    t.alloc(in_size);
    front_face.alloc(in_size);
    sphere_index.alloc(in_size);
    sphere_index.fill(-1);         // Init to illegal value
  }

  points       p;
  points       normal;
  Float::Array t;
  Float::Array front_face;
  Int::Array   sphere_index;
};


// Ray coordinates
points origin;
points direction;

// Sphere coordinates
points       center;
Float::Array radius;

// Hit record values
HitRecords hit_records;


MAYBE_UNUSED bool same_vec(int index, vec3 const &v, points const &pts, int bit_min = 0, bool show_log = true) {
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


MAYBE_UNUSED bool same_float(int index, float val, Float::Array &ret_f, int bit_min = 0) {
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


void kernels_init() {
  // Don't bother initializing kernel if not used.
  if (global::run_mode() != RunScalar) {
    kernel::init();
  }
}


void init_arrays(int num_spheres) {
  assert(ArraySize % global::samples_per_pixel() == 0); // Samples per pixel must be in same buffer

  uint32_t size = ArraySize; //global::num_rays();
  assert(size % 16 == 0);

  origin.alloc(size);
  direction.alloc(size);
  hit_records.alloc(size);

  s_num_spheres = resize_16(num_spheres);
  assert(s_num_spheres % 16 == 0);
  //warn << "s_num_spheres: " << s_num_spheres;

  center.alloc(s_num_spheres);
  radius.alloc(s_num_spheres);
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
 * **NOTE**: Material not added here
 *
 * This used to be used extensively, causing a significant performance hit.
 * Main loop for the testcase is about 4s without iti, instead of 22s.
 *
 * Timing inconsequential.
 */
sphere get_sphere(int index) {
  assert(0 <= index && index < num_spheres());
  auto ret = sphere(center.to_vec(index), (double) radius[index], nullptr);

  return ret;
}


bool same_sphere(int index, sphere const &s) {
  assert(0 <= index && index < num_spheres());

  int bit_min = -1;
  auto const &s0 = get_sphere(index);

  return same_vec(s0.center(), s.center(), bit_min)
      && bit_diff((float) s0.radius(), (float) s.radius(), bit_min);
}


void end() {
  auto percent = [] (int val) -> std::string {
    std::string ret;
    ret << (int) (100.0*val/s_total_matches) << "%";
    return ret;
  };

  warn << "\n"
       << "  Total        : " << s_total_matches << "\n"
       << "  exact matches: " << s_exact_match << ", " << percent(s_exact_match)
  ;
}


namespace {

void hittable_list_hit(const ray &r, int ray_index) {
  assert(s_num_spheres > 0);

  timers.start("hittable_list_hit");
  kernel::sphere_hit(
    r, ray_index,
    s_num_spheres,
    center.x, center.y, center.z,
    radius,
    hit_records.p.x, hit_records.p.y, hit_records.p.z,
    hit_records.normal.x, hit_records.normal.y, hit_records.normal.z,
    hit_records.t,
    hit_records.front_face,
    hit_records.sphere_index
  );
  timers.stop("hittable_list_hit");
}

} // anon namespace


void run_kernel() {
  assert(global::run_mode() != RunScalar);

  timers.start("QPU run");

  for (int index = 0; index < rays::num(); index++) {
    ray r = rays::get(index, false);
    hittable_list_hit(r, index);
  }

  timers.stop("QPU run");
}

}  // namespace qpu


namespace rays {
namespace {

// Index of first item in point array
// -1 indicates 'not filled'
int s_point_first_index = -1;

// Number of items in point arrays
int s_point_count = 0;


int relative_index(int ray_index) {
  int index = ray_index - s_point_first_index;
  assert(s_point_count > 0);
  assert(0 <= index && index < s_point_count);

  return index;
}

} // anon namespace


bool set(ray const &in_ray, int ray_index) {
  if (s_point_first_index == -1) {
    //warn << "set_ray resetting first index";
    assert(ray_index >= 0);
    s_point_first_index = ray_index;
    s_point_count = 0;
  } else {
    assert(s_point_count < qpu::ArraySize);
  }

  int index = ray_index - s_point_first_index;
  assert(0 <= index && index < qpu::ArraySize);

  qpu::origin.set_vec(index, in_ray.origin());
  qpu::direction.set_vec(index, in_ray.direction());

  s_point_count++;
  bool ret = (s_point_count < qpu::ArraySize);

  return ret;
}


/**
 * Timing inconsequential.
 */
ray get(uint32_t ray_index, bool absolute_index) {
  int index = ray_index;

  if (absolute_index) {
    index -= s_point_first_index;
  }

  assert(s_point_count > 0);
  assert(0 <= index && index < s_point_count);

  vec3 tmp_origin    = qpu::origin.to_vec(index);
  vec3 tmp_direction = qpu::direction.to_vec(index);

  return ray(tmp_origin, tmp_direction);
}


int num() {
  return s_point_count;
}


int first_index() {
  assert(s_point_first_index >=0);
  return s_point_first_index;
}


int last_index() {
  assert(s_point_first_index >=0);
  return s_point_first_index + s_point_count;
}


void reset() {
  s_point_first_index = -1;
  s_point_count = 0;
}

}  // namespace rays


bool same(ray const &lhs, ray const &rhs) {
  return qpu::same_vec(lhs.origin(), rhs.origin(), -1)
      && qpu::same_vec(lhs.direction(), rhs.direction(), -1);
}


namespace hit_records {

std::string dump(int index) {
  std::string ret;

  ret //<< index << ": "
      << "p: "      << qpu::hit_records.p.dump_vec(index)      << ", "
      << "normal: " << qpu::hit_records.normal.dump_vec(index) << ", "
      << "t: "      << qpu::hit_records.t[index] << ", "
      << "sphere_index: "      << qpu::hit_records.sphere_index[index];

  return ret;
}


void check(int index, hit_record const &rec) {
  auto t      = qpu::hit_records.t[index];
  auto p      = qpu::hit_records.p.to_vec(index);
  auto normal = qpu::hit_records.normal.to_vec(index);
  auto front_face = qpu::hit_records.front_face[index];

  bitdiff_stats::add(t            , (float) rec.t    , 15);
  bitdiff_stats::add((float) p.x(), (float) rec.p.x(), 16);
  bitdiff_stats::add((float) p.y(), (float) rec.p.y(), 17);
  bitdiff_stats::add((float) p.z(), (float) rec.p.z(), 18);
  bitdiff_stats::add((float) normal.x(), (float) rec.normal.x(), 19);
  bitdiff_stats::add((float) normal.y(), (float) rec.normal.y(), 20);
  bitdiff_stats::add((float) normal.z(), (float) rec.normal.z(), 21);
  bitdiff_stats::add(front_face        , rec.front_face?1.0f:-1.0f, 22);
}


hit_record get(int ray_index) {
  timers.start("hit_records::get");

  int index = rays::relative_index(ray_index);
  hit_record ret;

  auto p      = qpu::hit_records.p.to_vec(index);
  auto normal = qpu::hit_records.normal.to_vec(index);
  auto t      = qpu::hit_records.t[index];

  float tmp   = qpu::hit_records.front_face[index];
  assert(tmp == -1.0f || tmp == 1.0f);
  bool front_face = (tmp == 1.0f);

  int sphere_index = qpu::hit_records.sphere_index[index];
  assert(sphere_index >= 0);
  sphere const &s = spheres::get(sphere_index);

  ret.p          = p;
  ret.normal     = normal;
  ret.t          = t;
  ret.front_face = front_face;
  ret.mat        = s.mat();

  timers.stop("hit_records::get");

  return ret;
}  


bool valid(int ray_index) {
  int index =  rays::relative_index(ray_index);

  float val = qpu::hit_records.p.x[index];
  float inf = std::numeric_limits<float>::infinity();

  return val != inf && val != -inf;
}  

} // namespace hit_records
