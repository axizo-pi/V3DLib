#include "sphere.h"
#include "qpu.h"
#include "Support/basics.h"
#include <cmath>
//#include <cassert>

using namespace V3DLib;
using namespace Log;

sphere::sphere(const point3& center, double radius, shared_ptr<material> mat)
  : m_center(center), m_radius(std::fmax(0,radius)), m_mat(mat) {}


/**
 * **NOTE**: ray_t.min always 0.001 (verified)
 */
bool sphere::hit(const ray& r, interval ray_t, hit_record& rec, int ray_index, int sphere_index, bool qpu_check) const {
  //warn << "ray_index: " << ray_index;

  vec3 oc = m_center - r.origin();
  //OK assert(qpu::check_ret(sphere_index, oc));

  auto a = r.direction().f_length_squared();
  //OK assert(qpu::check_f(sphere_index, a));

  // The differences in calculation can thus be explained
  // in precision differences between double and float
  auto h = f_dot(r.direction(), oc);
  //OK assert(qpu::check_f(sphere_index, h, 0, 0));

  float rad = (float) m_radius;
  auto c = oc.f_length_squared() - rad*rad;
  //OK assert(qpu::check_f(sphere_index, c, 0));

  float discriminant = h*h - ((float) a)*c;
  //OK assert(qpu::check_f(sphere_index, discriminant));
  //OK assert(qpu::check_sign(sphere_index, discriminant)); // Sign important

  if (discriminant < 0) {
    return false;
  }

  auto sqrtd = std::sqrt(discriminant);
  //OK assert(qpu::check_f(sphere_index, sqrtd));

  // Find the nearest root that lies in the acceptable range.
  auto root = (h - sqrtd) / a;
  //OK, good enough assert(qpu::check_f(sphere_index, root, 2.6e-6f, 6));

  if (!ray_t.surrounds(root)) {
    root = (h + sqrtd) / a;
    if (!ray_t.surrounds(root))
      return false;
  }
  // OK, good enough assert(qpu::check_f(sphere_index, root, 2.6e-6f , 6));

  rec.t = root;
  rec.p = r.at(rec.t);
  //rec.p = r.f_at(rec.t);  // Float mult doesn't help

	// Not so good; appears to get worse for higher ray indexes.
	// assert(qpu::check_ret(sphere_index, rec.p, 3.0e-4f, 10));

  vec3 outward_normal = (rec.p - m_center) / m_radius;

	if (qpu_check) {
		std::string buf;
		buf  << "check_ret failed at "
		     << "ray_index: " << ray_index << ", "
			   << "sphere_index: " << sphere_index;

		vec3 zero(0,0,0);

  	if (qpu::check_ret(sphere_index, zero, 0, 0, false)) {
			//warn << buf << ": qpu is zero";
		} else if (!qpu::check_ret(sphere_index, outward_normal, 1.7e-5f, 10)) {
			warn << buf << "\n"
			     << "rec.p   : " << rec.p.dump() << "\n"	
					 << "m_center: " << m_center.dump() << "\n"
					 << "m_radius: " << m_radius;

				assert(false);
		}
	}

  rec.set_face_normal(r, outward_normal);
  rec.mat = m_mat;

  return true;
}


std::string sphere::dump() const {
  std::string ret;
  ret << "sphere center: " <<  m_center.dump() << ", radius: " << m_radius;
  return ret;
}
