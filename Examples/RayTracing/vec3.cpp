#include "vec3.h"
#include "rtweekend.h"

double vec3::length() const {
  return std::sqrt(length_squared());
}


/**
 * @brief Return true if the vector is close to zero in all dimensions.
 */
bool vec3::near_zero(double precision) const {
  return (std::fabs(e[0]) < precision) && (std::fabs(e[1]) < precision) && (std::fabs(e[2]) < precision);
}


vec3 vec3::random() {
  return vec3(random_double(), random_double(), random_double());
}


vec3 vec3::random(double min, double max) {
  return vec3(random_double(min,max), random_double(min,max), random_double(min,max));
}


vec3 random_in_unit_disk() {
  while (true) {
    auto p = vec3(random_double(-1,1), random_double(-1,1), 0);
    if (p.length_squared() < 1)
      return p;
  }
}


vec3 random_unit_vector() {
  while (true) {
    auto p = vec3::random(-1,1);
    auto lensq = p.length_squared();
    if (1e-160 < lensq && lensq <= 1.0)
      return p / sqrt(lensq);
  }
}


vec3 refract(const vec3& uv, const vec3& n, double etai_over_etat) {
  auto cos_theta = std::fmin(dot(-uv, n), 1.0);
  vec3 r_out_perp =  etai_over_etat * (uv + cos_theta*n);
  vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
  return r_out_perp + r_out_parallel;
}
