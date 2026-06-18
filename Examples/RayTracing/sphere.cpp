#include "sphere.h"
#include "qpu.h"
#include "Support/basics.h"
#include <cmath>
//#include <cassert>

using namespace V3DLib;
using namespace Log;

sphere::sphere(const point3& center, double radius, shared_ptr<material> mat)
  : m_center(center), m_radius(std::fmax(0,radius)), m_mat(mat) {}


bool sphere::hit(const ray& r, interval ray_t, hit_record& rec, int sphere_index, bool qpu_check) const {
	// warn << "sphere_index: " << sphere_index;

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
	//OK assert(qpu::check_f(sphere_index, c, 0)); //, 9));

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
	if (qpu_check) {
		//OK, good enough assert(qpu::check_f(sphere_index, root, 2.6e-6f, 6));
	}

	warn << "ray_t: " << ray_t.dump();
  if (!ray_t.surrounds(root)) {
    root = (h + sqrtd) / a;
    if (!ray_t.surrounds(root))
      return false;
  }

  rec.t = root;
  rec.p = r.at(rec.t);
  vec3 outward_normal = (rec.p - m_center) / m_radius;
  rec.set_face_normal(r, outward_normal);
  rec.mat = m_mat;

  return true;
}

std::string sphere::dump() const {
	std::string ret;

	ret << "sphere center: " <<  m_center.dump() << ", radius: " << m_radius;

	return ret;
}
