#include "hittable.h"

/**
 * @brief Sets the hit record normal vector.
 *
 * The parameter `outward_normal` is assumed to have unit length.
 */
void hit_record::set_face_normal(const ray& r, const vec3& outward_normal) {
  // The '< 0' confused me, but since the output image is correct, it must be ok.
  front_face = dot(r.direction(), outward_normal) < 0;
  normal = front_face ? outward_normal : -outward_normal;
}


std::string hit_record::dump() const {
	std::string ret;
	//ret << "p: " << p.dump();
	ret << "t: " << t;
	return ret;
}
