#ifndef _V3DLIB_GRAVITY_DEFAULTS_H
#define _V3DLIB_GRAVITY_DEFAULTS_H
//#include <random>

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
int const BATCH_STEPS = 150;

const int N_PLANETS   = 9;                       // Also includes the sun, pluto missing
const int N_ASTEROIDS = 23;
const int NUM         = N_PLANETS + N_ASTEROIDS; // Total number of gravitational entities

double const dt    = 86400;
double const DECADE = 86400 * 365 * 10; // approximately a decade in seconds
double const t_end = ((double) 35) * DECADE;
double const BIG_G = 6.67e-11; // gravitational constant, (m^3⋅kg^−1⋅s^−2)


#endif //  _V3DLIB_GRAVITY_DEFAULTS_H
