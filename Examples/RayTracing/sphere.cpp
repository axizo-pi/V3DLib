#include "sphere.h"
#include "qpu.h"
#include "Support/basics.h"
#include "Support/Platform.h" // compiling_for_vc4()
#include "Support/dump.h"
#include <cmath>
#include <iostream>

using namespace V3DLib;
using namespace Log;

namespace {

MAYBE_UNUSED void qpu_check_ret(vec3 const &v, int ray_index, int sphere_index) {
  timers.start("qpu_check ret");

  std::string buf;
  buf  << "check_ret failed at "
       << "ray_index: " << ray_index << ", "
       << "sphere_index: " << sphere_index;

  vec3 zero(0,0,0);
  vec3 negative = -1.0f*v;

  int zero_bit_diff    = 14;

  if (Platform::compiling_for_vc4()) {
    zero_bit_diff  = 18;
  }

   if (qpu::check_ret(sphere_index, zero, 0, false)) {
    //warn << buf << ": qpu is zero";
    qpu::add_zero();
  } else if (qpu::check_ret(sphere_index, negative, zero_bit_diff, false)) {
    //warn << buf << ": qpu is negative";
    qpu::add_negative();
  } else if (!qpu::check_ret(sphere_index, v)) { //, 1.7e-5f, 10)) {
    warn << buf << "\n";
    assert(false);
  }

  timers.stop("qpu_check ret");
}

} // anon namespace


sphere::sphere(const point3& center, double radius, shared_ptr<material> mat)
  : m_center(center), m_radius(std::fmax(0,radius)), m_mat(mat) {}


/**
 * @brief Do the scalar calculation for sphere hits.
 *
 *
 * This is also used to verify the QPU calculation, see **Note 1**.
 *
 * **NOTE**: ray_t.min always 0.001 (verified)
 *
 * @param qpu_check if true, compare scalar value with QPU value.
 *
 * -----------------------------------------------------------------
 *
 * Notes
 * -----
 *
 *1. When`qpu_check == true`, the QPU calculations are verified.
 *
 *   The interim assertions depend on debug return values (`ret_f, ret_x, ret_y, ret_z`) for that step.
 *   In other words, the kernel needs to be adjusted to return the correct interim value.
 *   Eventually, the debug return values will be removed, when I'm absolutely sure that the kernel
 *   works correctly.
 *
 *   To test a single float value, eg:
 *
 *     if (qpu_check) assert(qpu::check_f(sphere_index, a));
 *
 *   The second param (here `a`) depends on whatever value you are testing.
 *
 *   To test a vector directly:
 *
 *     if (qpu_check) assert(qpu::check_ret(sphere_index, oc));
 *
 *   There is also an internal routine, `check_vec()`, which does the same thing with
 *   extended information.
 */
bool sphere::hit(const ray& r, interval ray_t, hit_record& rec, int ray_index, int sphere_index, bool qpu_check) const {

  /**
   * @brief Local method to facilitate comparison of vectors.
   */
  MAYBE_UNUSED auto check_vec = [qpu_check, ray_index, sphere_index] (vec3 const &vec) {
    if (!qpu_check) return; 

    bool passed = qpu::check_ret(sphere_index, vec, 18);
    if (!passed) {
      warn << "FAIL ray_index: " << ray_index;
    }
//    assert(passed);
    auto qpu_vec = qpu::get_ret(sphere_index);

    if (!passed) {
      warn << "\n  vec    : " << vec.dump()
           << "\n  qpu_vec: " << qpu_vec.dump();
    }      

    bitdiff_stats::add((float) qpu_vec.x(), (float) vec.x(), 123);
    bitdiff_stats::add((float) qpu_vec.y(), (float) vec.y(), 123);
    bitdiff_stats::add((float) qpu_vec.z(), (float) vec.z(), 123);
  };

  //
  // Following values all exact in test
  //

  vec3 oc = m_center - r.origin();
  auto a = r.direction().f_length_squared();
  auto h = f_dot(r.direction(), oc);

  float rad = (float) m_radius;
  float c   = (float) (oc.f_length_squared() - rad*rad);

  float discriminant = h*h - a*c;

  // At this point, comparing `discriminant >= 0.0f` with `valid[ray_index]` returned
  // from the kernel is a perfect match.

  if (discriminant < 0.0f) {
    return false;
  }

  //
  // From here onwards, checking `qpu_check` is required for scalar-QPU comparison
  //

  float sqrtd = std::sqrt(discriminant);

  //
  // End values all exact in test
  //

  // Find the nearest root that lies in the acceptable range.
  auto root = (h - sqrtd) / a;
  // Gets worse for higher ray_index
  // Total: 36401; bitdiff: 87% <= 0, <22265, 9653, 693, 213, 1803, 55, 1719, rest 0>
  // OK, good enough: if (qpu_check) assert(qpu::check_f(sphere_index, root, 5));

  if (!ray_t.surrounds(root)) {
    root = (h + sqrtd) / a;
    // Comparing with root_2 in kernel
    // Total: 904; bitmax: 100% <= 1
    // Interestingly, this one is more precise than the '-' version
    // OK:
    if (qpu_check) {
      assert(qpu::check_f(sphere_index, root, 1));
      bitdiff_stats::add(qpu::get_f(sphere_index), (float) root, 1);
    }

    if (!ray_t.surrounds(root)) {
      //warn << "!surrounds 2";
      //assert(qpu::get_valid(sphere_index) == 0);
      return false;
    } else {
      if (qpu_check) {
        warn << "Success !surrounds 2";
      }
    }
  }

  assert(qpu::get_valid(sphere_index) == 1);

  rec.t = root;         // OK, same as root

  rec.p = r.at(rec.t);
  // if (qpu_check) { bool passed = qpu::check_ret(sphere_index, rec.p, 22); }
  // bitmax 92% <= 6, for xyz combined
  //
  // Biggest difference:
  //
  // ray_index: 11839
  // bit_diff(): (offset > ignore_bit): (23 > 22)
  //   vec    : vec3(1.387859e-01, -3.184897e-06, 1.453774e-01)
  //   qpu_vec: vec3(1.387978e-01, -1.430511e-06, 1.453800e-01)
  //
  // Largest diff in y-coord; note that it is on the order of 10e-6, much smaller that x and z.
  //


  vec3 outward_normal = (rec.p - m_center) / m_radius;
  // bitmax 95% <= 6
  // check_vec(outward_normal);
  //
  // Verified: qpu length values are never 0.0f, close enough to scalar lengths.
  // length bitmax 99% <= 6
  // auto qpu_vec = qpu::get_ret(sphere_index);
  // float len = (float) outward_normal.length();
  // float qpu_len = (float) qpu_vec.length();
  // bitdiff_stats::add(qpu_len, len, 112);

  rec.set_face_normal(r, outward_normal);
  // OK front_face perfect match
  // if (qpu_check) assert(qpu::check_f(sphere_index, (rec.front_face?1.0f:-1.0f), -1));
  // rec.normal bitmax 95% <= 6
  check_vec(rec.normal);

  rec.mat = m_mat;
  return true;
}


std::string sphere::dump() const {
  std::string ret;
  ret << "sphere center: " <<  m_center.dump() << ", radius: " << m_radius;
  return ret;
}
