#include "sphere.h"
#include "qpu.h"
#include "global/log.h"
#include <cmath>
//#include <cassert>

using namespace Log;

sphere::sphere(const point3& center, double radius, shared_ptr<material> mat)
  : m_center(center), m_radius(std::fmax(0,radius)), mat(mat) {}


bool sphere::hit(const ray& r, interval ray_t, hit_record& rec, int sphere_index) const {
	if (sphere_index == 0) 
		warn << "sphere_index: " << sphere_index;

  vec3 oc = m_center - r.origin();
	//OK assert(qpu::check_ret(sphere_index, oc));

  auto a = r.direction().length_squared();
	//OK assert(qpu::check_f(sphere_index, (float) a));

  auto h = dot(r.direction(), oc);
	//assert(qpu::check_f(sphere_index, (float) h)); // precision 1e-5

  auto c = oc.length_squared() - m_radius*m_radius;
	//warn << "c: " << c;
	assert(qpu::check_f(sphere_index, (float) c));

  auto discriminant = h*h - a*c;
  if (discriminant < 0)
    return false;

  auto sqrtd = std::sqrt(discriminant);

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
