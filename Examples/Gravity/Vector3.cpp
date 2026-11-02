#include "Vector3.h"
#include "Support/basics.h"
#include "tools.h"           //  frand()
#include <cmath>

Vector3::Vector3(double e0, double e1, double e2) {
  e[0] = e0;
  e[1] = e1;
  e[2] = e2;
}


std::string Vector3::dump() const {
  std::string ret;
  ret << "Vector3(" << (float) e[0] << ", " << (float) e[1] << ", " << (float) e[2] << ")";

  return ret;
}


double Vector3::length() const {
  double val = x()*x() + y()*y() + z()*z();
  return sqrt(val);
}


Vector3 Vector3::operator*(double val) const {
  auto  ret = *this;

  ret.e[0] *= val;
  ret.e[1] *= val;
  ret.e[2] *= val;

  return ret;
}


/**
 * Return a unit vector with random coordinates
 *
 * The coordinates follow a Gaussian distribution.
 */
Vector3 Vector3::random() {
  Vector3 ret(frand(), frand(), frand());
  //Log::warn << "Vector3::random() ret: " << ret.dump();

  auto length = ret.length();
  ret.e[0] /= length;
  ret.e[1] /= length;
  ret.e[2] /= length;

  return ret;
}


/**
 * @brief Store minimum values of coordinates in current instance.
 */
void Vector3::set_min(Vector3 const &rhs) {
  if (e[0] > rhs.e[0]) e[0] = rhs.e[0];
  if (e[1] > rhs.e[1]) e[1] = rhs.e[1];
  if (e[2] > rhs.e[2]) e[2] = rhs.e[2];
}


/**
 * @brief Store maximum values of coordinates in current instance.
 */
void Vector3::set_max(Vector3 const &rhs) {
  if (e[0] < rhs.e[0]) e[0] = rhs.e[0];
  if (e[1] < rhs.e[1]) e[1] = rhs.e[1];
  if (e[2] < rhs.e[2]) e[2] = rhs.e[2];
}


Vector3 operator*(double lhs, Vector3 const &rhs) {
  return rhs * lhs;
}
