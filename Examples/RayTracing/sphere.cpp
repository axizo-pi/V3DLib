#include "sphere.h"
#include "qpu.h"
#include "Support/basics.h"
#include <cmath>
//#include <cassert>

using namespace V3DLib;
using namespace Log;

sphere::sphere(const point3& center, double radius, shared_ptr<material> mat)
  : m_center(center), m_radius(std::fmax(0,radius)), mat(mat) {}


bool sphere::hit(const ray& r, interval ray_t, hit_record& rec, int sphere_index, bool qpu_check) const {
	// warn << "sphere_index: " << sphere_index;

  vec3 oc = m_center - r.origin();
	//OK assert(qpu::check_ret(sphere_index, oc));
	//warn << "oc: " << oc.dump();
	//warn << "r: " << r.dump();

  auto a = r.direction().length_squared();
	//OK assert(qpu::check_f(sphere_index, (float) a));

  auto h = dot(r.direction(), oc);
	// Expected this to be exact; most calculations are.
	// Unclear why there is a difference.
	// First ray indexes that fail: 23, 59, 190, 192
	// Always on sphere index 0
	//assert(qpu::check_f(sphere_index, (float) h, 1.0e-5f));

  auto c = oc.length_squared() - m_radius*m_radius;
	// Again, fully expecting this to be exact.
	// Again, most calcs *are* exact.
	//assert(qpu::check_f(sphere_index, (float) c, 0, 9));

  auto discriminant = h*h - a*c;
/*	
	if (qpu_check) {
		// Differences here to be expected, since variables deviate already
		// The deviance is horrendous, but the only important thing at this point is the sign
		assert(qpu::check_f(sphere_index, (float) discriminant, 1.0e0f, 12));

		// Thankfully, sign checks out fine
		assert(qpu::check_sign(sphere_index, discriminant));
	}
*/	

  if (discriminant < 0) {
		if (qpu_check) {
			assert(qpu::check_f(sphere_index, 0.0f));
		}
		return false;
	}

  auto sqrtd = std::sqrt(discriminant);
	if (qpu_check) {
		assert(qpu::check_f(sphere_index, (float) sqrtd, 3.0e-3f, 9));
	}

  // Find the nearest root that lies in the acceptable range.
  auto root = (h - sqrtd) / a;
  if (!ray_t.surrounds(root)) {
    root = (h + sqrtd) / a;
    if (!ray_t.surrounds(root))
      return false;
  }

  rec.t = root;
  rec.p = r.at(rec.t);
  vec3 outward_normal = (rec.p - m_center) / m_radius;
  rec.set_face_normal(r, outward_normal);
  rec.mat = mat;

  return true;
}

std::string sphere::dump() const {
	std::string ret;

	ret << "sphere center: " <<  m_center.dump() << ", radius: " << m_radius;

	return ret;
}
