#include "ray.h"
#include "Support/basics.h"

std::string ray::dump() const {
  std::string ret;
  ret << "ray "
      << "origin: (" << origin().x() << ", " << origin().y() << ", " << origin().z() << "), "
      << "direction: (" << direction().x() << ", " << direction().y() << ", " << direction().z() << ")";

  return ret;
}


ray ray::abs_diff(ray const &rhs) const {
  point3 diff_orig(
    std::abs(origin().x() - rhs.origin().x()),
    std::abs(origin().y() - rhs.origin().y()),
    std::abs(origin().z() - rhs.origin().z())
  );
  diff_orig /= origin().length();  // Make relative to lhs

  vec3 diff_dir(
    std::abs(direction().x() - rhs.direction().x()),
    std::abs(direction().y() - rhs.direction().y()),
    std::abs(direction().z() - rhs.direction().z())
  );
  diff_dir /= direction().length();  // Make relative to lhs

  ray ret(diff_orig, diff_dir);
  return ret;
}
