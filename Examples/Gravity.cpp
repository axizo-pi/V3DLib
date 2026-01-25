#include "V3DLib.h"
#include "Support/Settings.h"
#include "Support/bmp.h"
#include "Support/Timer.h"
#include <cmath>
#include <random>

using namespace V3DLib;
using namespace Log;

V3DLib::Settings settings;

/*
// Source: https://stackoverflow.com/a/33389729/1223531
std::default_random_engine generator;
std::uniform_real_distribution<double> distribution(-1,1); //doubles from -1 to 1
double rand_1() { return distribution(generator); }
*/


/*
Float min/max = +/-3.40282e+38

F = BIG_G*m1*m2/r^2  = (m^3⋅kg^−1⋅s^−2)*kg^2/m^2 = kg*m/s^2
		= (m^3⋅kg^−1⋅s^−2)*Pg^2/Gm^2 * 1e24/1e18
*/

float const IMG_CONVERSION_FACTOR = 2.5e13f;

const int N_PLANETS   = 9;                       // Also includes the sun, pluto missing
const int N_ASTEROIDS = 23;
const int NUM         = N_PLANETS + N_ASTEROIDS; // Total number of gravitational entities

double t_0   = 0;
double t     = t_0;
double dt    = 86400;
double const DECADE = 86400 * 365 * 10; // approximately a decade in seconds
double t_end = ((double) 30) * DECADE;
double BIG_G = 6.67e-11; // gravitational constant, (m^3⋅kg^−1⋅s^−2)


///////////////////////////////////////////////////////////////
// Scalar simulation
// Source: https://en.wikipedia.org/wiki/N-body_simulation
///////////////////////////////////////////////////////////////

struct Vector3 {
  double e[3] = { 0 };

  Vector3() {}
  ~Vector3() {}

  inline Vector3(double e0, double e1, double e2) {
    this->e[0] = e0;
    this->e[1] = e1;
    this->e[2] = e2;
  }

	std::string dump() const {
		std::string ret;
		ret << "Vector3(" << (float) e[0] << ", " << (float) e[1] << ", " << (float) e[2] << ")";

		return ret;
	}
};

struct OrbitalEntity {
  double e[7] = { 0 };

  OrbitalEntity() {}
  ~OrbitalEntity() {}

  inline OrbitalEntity(double e0, double e1, double e2, double e3, double e4, double e5, double e6) {
    this->e[0] = e0;
    this->e[1] = e1;
    this->e[2] = e2;
    this->e[3] = e3;
    this->e[4] = e4;
    this->e[5] = e5;
    this->e[6] = e6;
  }

	std::string dump() const {
		std::string ret;

		ret << "Position(" << (float) e[0] << ", " << (float) e[1] << ", " << (float) e[2] << "), "
		    << "Velocity(" << (float) e[3] << ", " << (float) e[4] << ", " << (float) e[5] << "), "
		    << "Mass: " << (float) e[6];

		return ret;
	}

	double x() const { return e[0]; }
	double y() const { return e[1]; }
	double z() const { return e[2]; }
};


std::vector<OrbitalEntity> orbital_entities(NUM);

void init_orbital_entities() {
	auto &o = orbital_entities;

	o[0] = { 0.0,0.0,0.0,        0.0,0.0,0.0,      1.989e30 };   // a star similar to the sun
	o[1] = { 57.909e9,0.0,0.0,   0.0,47.36e3,0.0,  0.33011e24 }; // a planet similar to mercury
	o[2] = { 108.209e9,0.0,0.0,  0.0,35.02e3,0.0,  4.8675e24 };  // a planet similar to venus
	o[3] = { 149.596e9,0.0,0.0,  0.0,29.78e3,0.0,  5.9724e24 };  // a planet similar to earth
	o[4] = { 227.923e9,0.0,0.0,  0.0,24.07e3,0.0,  0.64171e24 }; // a planet similar to mars
	o[5] = { 778.570e9,0.0,0.0,  0.0,13e3,0.0,     1898.19e24 }; // a planet similar to jupiter
	o[6] = { 1433.529e9,0.0,0.0, 0.0,9.68e3,0.0,   568.34e24 };  // a planet similar to saturn
	o[7] = { 2872.463e9,0.0,0.0, 0.0,6.80e3,0.0,   86.813e24 };  // a planet similar to uranus
	o[8] = { 4495.060e9,0.0,0.0, 0.0,5.43e3,0.0,   102.413e24 }; // a planet similar to neptune

	assert(N_ASTEROIDS >= 0);
	for (int i = 0; i < N_ASTEROIDS; ++i) {
		o[N_PLANETS + i] = {
			8e12 + 9e10*i, 0, 0,
			0, 3.5e3, 0,
			1e22
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

  for (size_t entity_idx = 0; entity_idx < 9 + N_ASTEROIDS; entity_idx++) {
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
	while (t < t_end) {
		scalar_step();
		plot_orbital_entities(img);

    t += dt;
	}
}


///////////////////////////////////////////////////////////////
// Kernel version 
///////////////////////////////////////////////////////////////

/**
 * Pre: num_entities multiple of 16
 */
void kernel(
	Float::Ptr in_x,
	Float::Ptr in_y,
	Float::Ptr in_z,
 	Float::Ptr in_mass,
 	Float::Ptr out_acc_x,
 	Float::Ptr out_acc_y,
 	Float::Ptr out_acc_z,
	Int num_entities
) {
	Float DIST_FACTOR = 1e-12f;  comment("Init DIST_FACTOR");
	Float MASS_FACTOR = 1e-18f;  comment("Init MASS_FACTOR");

	For (Int cur_index = 0, cur_index < num_entities, cur_index++)
		Float::Ptr px0    = in_x    - index() + cur_index;
		Float::Ptr py0    = in_y    - index() + cur_index;
		Float::Ptr pz0    = in_z    - index() + cur_index;
		Float::Ptr pmass0 = in_mass - index() + cur_index;

		Float x0    = *px0    * DIST_FACTOR;
		Float y0    = *py0    * DIST_FACTOR;
		Float z0    = *pz0    * DIST_FACTOR;
		Float mass0 = *pmass0 * MASS_FACTOR;

		// Accumulated acceleration vector
		Float x0_acc = 0;
		Float y0_acc = 0;
		Float z0_acc = 0;

		Float::Ptr px    = in_x;
		Float::Ptr py    = in_y;
		Float::Ptr pz    = in_z;
		Float::Ptr pmass = in_mass;

		For (Int cur_block = 0, cur_block < num_entities/16, cur_block++)
			Float x    = *px    * DIST_FACTOR;
			Float y    = *py    * DIST_FACTOR;
			Float z    = *pz    * DIST_FACTOR;
			Float mass = *pmass * MASS_FACTOR;

			// Acceleration vector
			Float nx1 = x0 - x;          comment("Calc acceleration vector");
			Float ny1 = y0 - y; 
			Float nz1 = z0 - z; 

			// Gravity/Acceleration components
			Float dx1 = nx1 * nx1;
			Float dy1 = ny1 * ny1;
			Float dz1 = nz1 * nz1;

			Float d1 = dx1 + dy1 + dz1;  comment("Calc distance denominator");
			//d1 = d1 * 1e12f;
			//d1 = 3.6e12f;
			

			// Set the force/acceleration to zero for current entity,
			// calculate the rest.
			Float denom = 0;
			Float acceleration = 0;

			Where ((cur_block*16 + index()) != cur_index)
				denom = recipsqrt(d1);
				acceleration = -1 * mass * recip(d1);
			End

			acceleration *= ((float) BIG_G) * DIST_FACTOR / MASS_FACTOR * DIST_FACTOR;
			comment("Calc force factor");

			// Normalize the acceleration vector
			nx1 *= denom;  comment("Normalize the acc vector"); 
			ny1 *= denom; 
			nz1 *= denom; 

			// Set the force component
			nx1 *= acceleration; 
			ny1 *= acceleration; 
			nz1 *= acceleration; 
			

			// Add to total
			x0_acc += nx1;
			y0_acc += ny1;
			z0_acc += nz1;

			// Do next iteration
			px.inc();
			py.inc();
			pz.inc();
			pmass.inc();
		End

		// Sum up and return the acceleration
		rotate_sum(x0_acc, x0_acc);
		Float::Ptr pacc_x = out_acc_x - index() + cur_index;
		*pacc_x = x0_acc;

		rotate_sum(y0_acc, y0_acc);
		Float::Ptr pacc_y = out_acc_y - index() + cur_index;
		*pacc_y = y0_acc;

		rotate_sum(z0_acc, z0_acc);
		Float::Ptr pacc_z = out_acc_z - index() + cur_index;
		*pacc_z = z0_acc;
	End
}


/**
 * Pre: num_entities multiple of 16
 */
void kernel_step(
	Float::Ptr x,
	Float::Ptr y,
	Float::Ptr z,
	Float::Ptr in_v_x,
	Float::Ptr in_v_y,
	Float::Ptr in_v_z,
 	Float::Ptr acc_x,
 	Float::Ptr acc_y,
 	Float::Ptr acc_z,
	Int num_entities
) {
	Float delta_t = (float) dt;

	//
	// Update speed
	//
	Float::Ptr v_x = in_v_x;
	Float::Ptr v_y = in_v_y;
	Float::Ptr v_z = in_v_z;

	For (Int cur_block = 0, cur_block < num_entities/16, cur_block++)
		Float tmp_x = *v_x + *acc_x * delta_t;
		*v_x = tmp_x;

		Float tmp_y = *v_y + *acc_y * delta_t;
		*v_y = tmp_y;

		Float tmp_z = *v_z + *acc_z * delta_t;
		*v_z = tmp_z;

		v_x.inc();
		v_y.inc();
		v_z.inc();
		acc_x.inc();
		acc_y.inc();
		acc_z.inc();
	End


	//
	// Update position
	//
	v_x = in_v_x;
	v_y = in_v_y;
	v_z = in_v_z;

	For (Int cur_block = 0, cur_block < num_entities/16, cur_block++)
		Float tmp_x = *x + *v_x * delta_t;
		*x = tmp_x;

		Float tmp_y = *y + *v_y * delta_t;
		*y = tmp_y;

		Float tmp_z = *z + *v_z * delta_t;
		*z = tmp_z;

		x.inc();
		y.inc();
		z.inc();
		v_x.inc();
		v_y.inc();
		v_z.inc();
	End
}


struct Model {
	Float::Array x{NUM};
	Float::Array y{NUM};
	Float::Array z{NUM};
	Float::Array v_x{NUM};
	Float::Array v_y{NUM};
	Float::Array v_z{NUM};
	Float::Array mass{NUM};

	Float::Array acc_x{NUM};
	Float::Array acc_y{NUM};
	Float::Array acc_z{NUM};

	Image img{512, 512};

	//
	// Take the values from the scalar simulation, so that results can 
	// be compared.
	//
	// The main consideration here is that scalar uses doubles, which
	// do not exist on the QPU. QPU will use 4-byte Floats.
	// Differences in output are inevitable.
	//
	void init() {
		assert(NUM % 16 == 0);  // Num entities must be a multiple of 16

		auto &o = orbital_entities;

	  for (int i = 0; i < NUM; i++) {
			x[i]    = (float) o[i].x();
			y[i]    = (float) o[i].y();
			z[i]    = (float) o[i].z();
			v_x[i]  = (float) o[i].e[3];
			v_y[i]  = (float) o[i].e[4];
			v_z[i]  = (float) o[i].e[5];
			mass[i] = (float) o[i].e[6];
		}

		img.set_conversion_factor(IMG_CONVERSION_FACTOR);

		acc_x.fill(0);
		acc_y.fill(0);
		acc_z.fill(0);
	}

	void plot() {
		for (int i = 0; i < (int) x.size(); ++i) {
			img.plot(x[i], y[i]);
		}
	}

	void save_img() {
		img.save("gravity_qpu.bmp");
	}


	std::string dump_pos() const {
		std::string ret;

		ret << "x: " << x.dump() << "\n"
	  	  << "y: " << y.dump() << "\n"
				<< "z: " << z.dump() << "\n";

		return ret;
	}

	std::string dump_acc() const {
		std::string ret;

		ret << "acc_x: " << acc_x.dump() << "\n"
	  	  << "acc_y: " << acc_y.dump() << "\n"
				<< "acc_z: " << acc_z.dump() << "\n";

		return ret;
	}
};


///////////////////////////////////////////////////////////////
// Main 
///////////////////////////////////////////////////////////////

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

	Image img(512, 512);
	img.set_conversion_factor(IMG_CONVERSION_FACTOR);

	//
	// Run scalar version
	//
	{
		Timer timer("Scalar timer", true);

		init_orbital_entities();
		scalar_run(img);

		img.save("gravity.bmp");
	}


	//
	// Run kernel version
	//
	init_orbital_entities();
	Model m;
	m.init();
	m.plot();
	warn << m.dump_pos();

	auto k = compile(kernel, settings);
	k.load(&m.x, &m.y, &m.z, &m.mass, &m.acc_x, &m.acc_y, &m.acc_z, NUM);

	auto k_step = compile(kernel_step, settings);
	k_step.load(&m.x, &m.y, &m.z, &m.v_x, &m.v_y, &m.v_z, &m.acc_x, &m.acc_y, &m.acc_z, NUM);

	//if (false)
 	{
		Timer timer("QPU timer", true);

		t = 0;
		while (t < t_end) {
	  	k.run();
		  k_step.run();
			m.plot();

  	  t += dt;
		}

		warn << m.dump_acc();
		warn << m.dump_pos();
		m.save_img();
	}

  return 0;
}
