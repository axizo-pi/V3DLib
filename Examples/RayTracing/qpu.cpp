#include "qpu.h"
#include "global.h"
#include "kernel.h"
#include "Support/Helpers.h"  // resize_16()
#include "Support/dump.h"     // bitdiff_stats()
#include "Support/Timer.h"
#include "material.h"         // For default material
#include <limits>             // infinity

using namespace V3DLib;

namespace {

/**
 * Size of point arrays.
 *
 * Output fails for multi-ray kernel if the array size is selected too large.
 * I can not determine why this happens (overflow? I/O is swamped?).
 *
 * The single-ray kernel works fine. This test done on pi5 (vc7), rest not done yet.
 *
 * The workaround for now is to limit the array sizes here.
 */
const int ArraySize = 12800; //Highest value I could determine that succeeds. Fail: 20480;
//const int ArraySize = 92160; // Decent heuristic, which fits into the default heap size.

int s_exact_match   = 0;
int s_total_matches = 0;

struct points {
  void alloc(int in_size, float init_val = 0.0f) {
    size = in_size;
    assert(size > 0);

    x.alloc(size);
    y.alloc(size);
    z.alloc(size);

    x.fill(init_val);
    y.fill(init_val);
    z.fill(init_val);
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
    p.alloc(in_size, 123.0f);
    normal.alloc(in_size);
    t.alloc(in_size);
    front_face.alloc(in_size);
    sphere_index.alloc(in_size);
    sphere_index.fill(-2);         // Init to illegal value
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

// Hit record values
HitRecords hitrecords;  // Var name hit_records was ambiguous with namespace hit_records


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

int s_num_spheres   = 0;

// Sphere coordinates
points       center;
Float::Array radius;

} // anon namespace


namespace spheres {

int num() {
  assert(s_num_spheres > 0);
  return s_num_spheres;
}


void add(int index, sphere const &in_sphere) {
  //warn << "add_sphere index: " << index << ", num spheres: " << num_spheres();
  assert(0 <= index && index < num());

  center.set_vec(index, in_sphere.center());
  radius[index] = (float) in_sphere.radius();
}


/**
 * @brief Get sphere from array
 *
 * **TODO**: See if this method can be removed in favor of spheres::get().
 *
 * **NOTE**: Material not added here
 *
 * This used to be used extensively, causing a significant performance hit.
 * Main loop for the testcase is about 4s without iti, instead of 22s.
 *
 * Timing inconsequential.
 */
sphere get_a(int index) {
  assert(0 <= index && index < num());
  auto ret = sphere(center.to_vec(index), (double) radius[index], nullptr);

  return ret;
}


bool same(int index, sphere const &s) {
  assert(0 <= index && index < spheres::num());

  int bit_min = -1;
  auto const &s0 = spheres::get_a(index);
  // TODO auto const &s0 = spheres::get(index); - Is this better (faster)?

  return same_vec(s0.center(), s.center(), bit_min)
      && bit_diff((float) s0.radius(), (float) s.radius(), bit_min);
}

}  // namespace spheres


namespace qpu {

void kernels_init() {
  // Don't bother initializing kernel if not used.
  if (global::run_mode() != RunScalar) {
    kernel::init();
  }
}


void init_arrays(int num_spheres) {
  assert(ArraySize % global::samples_per_pixel() == 0); // Samples per pixel must be in same buffer

  uint32_t size = ArraySize;
  assert(size % 16 == 0);

  origin.alloc(size);
  direction.alloc(size);
  hitrecords.alloc(size);

  s_num_spheres = resize_16(num_spheres);
  assert(s_num_spheres % 16 == 0);

  center.alloc(s_num_spheres);
  radius.alloc(s_num_spheres);
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

void hittable_list_hit(int ray_index) {
  assert(s_num_spheres > 0);
  //warn << "hittable_list_hit ray_num: " << rays::num();

  kernel::sphere_hit(
    ray_index,
    rays::num(),
    origin.x, origin.y, origin.z,
    direction.x, direction.y, direction.z,
    s_num_spheres,
    center.x, center.y, center.z,
    radius,
    hitrecords.p.x, hitrecords.p.y, hitrecords.p.z,
    hitrecords.normal.x, hitrecords.normal.y, hitrecords.normal.z,
    hitrecords.t,
    hitrecords.front_face,
    hitrecords.sphere_index
  );
}

} // anon namespace


void run_kernel() {
  assert(global::run_mode() != RunScalar);
  timers.start("QPU run");

#ifdef SINGLE_RAY
  warn << "SINGLE_RAY defined qpu";

  for (int index = 0; index < rays::num(); index++) {
    hittable_list_hit(index);
  }
#else
  hittable_list_hit(0);
#endif    

  timers.stop("QPU run");
  //sleep(10);
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
    assert(s_point_count < ArraySize);
  }

  int index = ray_index - s_point_first_index;
  assert(0 <= index && index < ArraySize);

  origin.set_vec(index, in_ray.origin());
  direction.set_vec(index, in_ray.direction());

  s_point_count++;
  bool ret = (s_point_count < ArraySize);

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

  vec3 tmp_origin    = origin.to_vec(index);
  vec3 tmp_direction = direction.to_vec(index);

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
  return same_vec(lhs.origin(), rhs.origin(), -1)
      && same_vec(lhs.direction(), rhs.direction(), -1);
}


namespace hit_records {

std::string dump(int index) {
  std::string ret;

  ret //<< index << ": "
      << "p: "            << hitrecords.p.dump_vec(index)      << ", "
      << "normal: "       << hitrecords.normal.dump_vec(index) << ", "
      << "t: "            << hitrecords.t[index] << ", "
      << "sphere_index: " << hitrecords.sphere_index[index];

  return ret;
}


void check(int index, hit_record const &rec) {
  auto t          = hitrecords.t[index];
  auto p          = hitrecords.p.to_vec(index);
  auto normal     = hitrecords.normal.to_vec(index);
  auto front_face = hitrecords.front_face[index];

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

  auto p      = hitrecords.p.to_vec(index);
  auto normal = hitrecords.normal.to_vec(index);
  auto t      = hitrecords.t[index];

  bool  front_face = false;
  float tmp        = hitrecords.front_face[index];
  if (tmp == -1.0f || tmp == 1.0f) {
    assert(tmp == -1.0f || tmp == 1.0f);
    front_face = (tmp == 1.0f);
  } else {
    //warn << "index: " << index << ", front_face: " << tmp;
  }

  int sphere_index = hitrecords.sphere_index[index];
  //warn << "index: " << index << ", sphere_index: " << sphere_index << ", p: " << p.dump();

  std::shared_ptr<material> mat = std::make_shared<dielectric>(1.5);

  if (sphere_index >= 0 && sphere_index < spheres::size()) {
    sphere const &s = spheres::get(sphere_index);
    mat = s.mat();
  } else {
    //warn << "index: " << index << ", sphere_index: " << sphere_index << ", p: " << p.dump();
  }

  ret.p          = p;
  ret.normal     = normal;
  ret.t          = t;
  ret.front_face = front_face;
  ret.mat        = mat;

  timers.stop("hit_records::get");

/*
  warn << "hitrecords::get(" << index << "), "
       << "sphere_index: " << sphere_index << ", "
       << "ret: " << ret.dump();
*/
  if (sphere_index < 0) {
    warn << "hitrecords::get(" << index << "), "
         << "sphere_index: " << sphere_index;
  }
  return ret;
}  


bool valid(int ray_index) {
  int index =  rays::relative_index(ray_index);

  float val = hitrecords.p.x[index];
  float inf = std::numeric_limits<float>::infinity();

  return val != inf && val != -inf;
}  

} // namespace hit_records
