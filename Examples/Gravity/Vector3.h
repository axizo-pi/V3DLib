#ifndef _V3DLIB_GRAVITY_VECTOR3_H
#define _V3DLIB_GRAVITY_VECTOR3_H
#include <string>

struct Vector3 {
  double e[3] = { 0 };

  Vector3() {}
  Vector3(double e0, double e1, double e2);
  Vector3(float const *p_e);
  ~Vector3() {}

  double x() const { return e[0]; }
  double y() const { return e[1]; }
  double z() const { return e[2]; }
  double length() const;

  double dot(Vector3 const &rhs) const;
  double dot() const;
  Vector3 &operator*=(double val);
  Vector3 operator*(double val) const;
  Vector3 &operator/=(double val);
  Vector3 operator/(double val) const;
  Vector3 &operator+=(Vector3 const &rhs);
  Vector3 operator+(Vector3 const &rhs) const;
  Vector3 &operator-=(Vector3 const &rhs);
  Vector3 operator-(Vector3 const &rhs) const;

  void set_min(Vector3 const &rhs);
  void set_max(Vector3 const &rhs);

  std::string dump() const;
  static Vector3 random();
};

Vector3 operator*(double lhs, Vector3 const &rhs);

#endif  // _V3DLIB_GRAVITY_VECTOR3_H
