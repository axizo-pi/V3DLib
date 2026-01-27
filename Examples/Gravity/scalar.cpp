#include "scalar.h"
#include "tools.h"
#include <cmath>
#include "Support/basics.h"

using namespace std;


/////////////////////////////////////////////////////////////////
// Class Vector3
/////////////////////////////////////////////////////////////////

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


Vector3 operator*(double lhs, Vector3 const &rhs) {
	return rhs * lhs;
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


/////////////////////////////////////////////////////////////////
// Class OrbitalEntity
/////////////////////////////////////////////////////////////////

OrbitalEntity::OrbitalEntity(double e0, double e1, double e2, double e3, double e4, double e5, double e6) {
  e[0] = e0;
  e[1] = e1;
  e[2] = e2;
  e[3] = e3;
  e[4] = e4;
  e[5] = e5;
  e[6] = e6;
}


OrbitalEntity::OrbitalEntity(Vector3 const &pos, Vector3 const &velocity, double mass) {
  e[0] = pos.x();
  e[1] = pos.y();
  e[2] = pos.z();
  e[3] = velocity.x();
  e[4] = velocity.y();
  e[5] = velocity.z();
  e[6] = mass;
}


std::string OrbitalEntity::dump() const {
	std::string ret;
	ret << "Position(" << (float) e[0] << ", " << (float) e[1] << ", " << (float) e[2] << "), "
	    << "Velocity(" << (float) e[3] << ", " << (float) e[4] << ", " << (float) e[5] << "), "
	    << "Mass: " << (float) e[6];

	return ret;
}


/////////////////////////////////////////////////////////////////
// Other Stuff 
/////////////////////////////////////////////////////////////////


std::vector<OrbitalEntity> orbital_entities(NUM);

void init_orbital_entities() {
	auto &o = orbital_entities;

	o[0] = { 0.0,0.0,0.0,        0.0,0.0,0.0,      1.989e30   };  // a star similar to the sun
	o[1] = { 57.909e9,0.0,0.0,   0.0,47.36e3,0.0,  0.33011e24 }; // a planet similar to mercury
	o[2] = { 108.209e9,0.0,0.0,  0.0,35.02e3,0.0,  4.8675e24  }; // a planet similar to venus
	o[3] = { 149.596e9,0.0,0.0,  0.0,29.78e3,0.0,  5.9724e24  }; // a planet similar to earth
	o[4] = { 227.923e9,0.0,0.0,  0.0,24.07e3,0.0,  0.64171e24 }; // a planet similar to mars
	o[5] = { 778.570e9,0.0,0.0,  0.0,13e3,0.0,     1898.19e24 }; // a planet similar to jupiter
	o[6] = { 1433.529e9,0.0,0.0, 0.0,9.68e3,0.0,   568.34e24  }; // a planet similar to saturn
	o[7] = { 2872.463e9,0.0,0.0, 0.0,6.80e3,0.0,   86.813e24  }; // a planet similar to uranus
	o[8] = { 4495.060e9,0.0,0.0, 0.0,5.43e3,0.0,   102.413e24 }; // a planet similar to neptune

	assert(N_ASTEROIDS >= 0);
	double const Asteroid_Dist  = 8e12;
	double const Asteroid_Speed = 3.5e3;
	double const Asteroid_Mass  = 1e22;

	for (int i = 0; i < N_ASTEROIDS; ++i) {
/*		
		o[N_PLANETS + i] = {
			Asteroid_Dist*Vector3::random(),
			Asteroid_Speed*Vector3::random(),
			Asteroid_Mass
		};
*/		
		o[N_PLANETS + i] = {
			Asteroid_Dist + 9e10*i, 0, 0,
			0, Asteroid_Speed, 0,
			Asteroid_Mass
		};
	}
}


void scalar_step() {
	auto &o = orbital_entities;

  for (size_t m1_idx = 0; m1_idx < NUM; m1_idx++) {
    Vector3 a_g = { 0,0,0 };

    for (size_t m2_idx = 0; m2_idx < NUM; m2_idx++) {
      if (m2_idx != m1_idx) {
        Vector3 r_vector;

        r_vector.e[0] = o[m1_idx].e[0] - o[m2_idx].e[0];
        r_vector.e[1] = o[m1_idx].e[1] - o[m2_idx].e[1];
        r_vector.e[2] = o[m1_idx].e[2] - o[m2_idx].e[2];

        double r_mag = sqrt(
          r_vector.e[0] * r_vector.e[0]
          + r_vector.e[1] * r_vector.e[1]
          + r_vector.e[2] * r_vector.e[2]);

        double acceleration = -1.0 * BIG_G * (o[m2_idx].e[6]) / pow(r_mag, 2.0);

        Vector3 r_unit_vector = {
					r_vector.e[0] / r_mag,
					r_vector.e[1] / r_mag,
					r_vector.e[2] / r_mag
				};
				//warn << "r_unit_vector: " << r_unit_vector.dump();

        a_g.e[0] += acceleration * r_unit_vector.e[0];
        a_g.e[1] += acceleration * r_unit_vector.e[1];
        a_g.e[2] += acceleration * r_unit_vector.e[2];
				//warn << "a_g: " << a_g.dump();
      }
    }

    o[m1_idx].e[3] += a_g.e[0] * dt;
    o[m1_idx].e[4] += a_g.e[1] * dt;
    o[m1_idx].e[5] += a_g.e[2] * dt;
  }

  for (size_t entity_idx = 0; entity_idx < NUM; entity_idx++) {
    o[entity_idx].e[0] += o[entity_idx].e[3] * dt;
    o[entity_idx].e[1] += o[entity_idx].e[4] * dt;
    o[entity_idx].e[2] += o[entity_idx].e[5] * dt;
  }
}


void plot_orbital_entities(Image &img) {
	auto &o = orbital_entities;

	for (int i = 0; i < (int) o.size(); ++i) {
		img.plot(o[i].x(), o[i].y());
	}
}


void scalar_run(Image &img) {
	double t = 0;

	while (t < t_end) {
		for (int i = 0; i < BATCH_STEPS; ++i) {
			scalar_step();
		}
		plot_orbital_entities(img);

    t += BATCH_STEPS*dt;
	}
}
