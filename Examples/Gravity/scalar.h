#ifndef _V3DLIB_GRAVITY_SCALAR_H
#define _V3DLIB_GRAVITY_SCALAR_H
#include "defaults.h"
#include "Support/bmp.h"
#include <string>
#include <vector>

/////////////////////////////////////////////////////////////////
// Scalar simulation
// Derived From: https://en.wikipedia.org/wiki/N-body_simulation
/////////////////////////////////////////////////////////////////

struct Vector3 {
  double e[3] = { 0 };

  Vector3() {}
  Vector3(double e0, double e1, double e2);
  ~Vector3() {}

	std::string dump() const;
};


struct OrbitalEntity {
  double e[7] = { 0 };

  OrbitalEntity() {}
  OrbitalEntity(double e0, double e1, double e2, double e3, double e4, double e5, double e6);
  ~OrbitalEntity() {}

	std::string dump() const;

	double x() const { return e[0]; }
	double y() const { return e[1]; }
	double z() const { return e[2]; }
};

extern std::vector<OrbitalEntity> orbital_entities;

void init_orbital_entities();
void scalar_step();
void plot_orbital_entities(Image &img);
void scalar_run(Image &img);


#endif // _V3DLIB_GRAVITY_SCALAR_H
