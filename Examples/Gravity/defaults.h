#ifndef _V3DLIB_GRAVITY_DEFAULTS_H
#define _V3DLIB_GRAVITY_DEFAULTS_H

/**
 * 27-1-26: Scalar vs. single QPU
 * - 96 entities for 25 decades:
 *   WARNING: Scalar timer:   60.621603s
 *   WARNING: QPU timer   :   28.327675s - output different due to rand()
 *
 * - 160 entities for 25 decades:
 *   WARNING: Scalar timer:  167.632421s
 *   WARNING: QPU timer   :   71.769218s
 *
 * - 288 entities for 25 decades:
 *   WARNING: Scalar timer: 548.515168s
 *   WARNING: QPU timer   : 217.725386s
 *
 * - 1048 entities for 25 decades:
 *	 WARNING: Scalar timer: 7360.986145s
 *	 WARNING: QPU timer   :  311.831073s  - No output, error?! Timeout I presume
 */

float const IMG_CONVERSION_FACTOR = 2.5e13f;

const int N_PLANETS   = 9;                       // Also includes the sun, pluto missing
const int N_ASTEROIDS = 23;
const int NUM         = N_PLANETS + N_ASTEROIDS; // Total number of gravitational entities

double const dt    = 86400;                      // 1 day
double const DECADE = 86400 * 365 * 10;          // approximately a decade in seconds
double const t_end = ((double) 1 /*25*/) * DECADE;     // End time for calculation. Default: 250 years.
double const BIG_G = 6.67e-11;                   // gravitational constant, (m^3⋅kg^−1⋅s^−2)

int const BATCH_STEPS = 1;                       // for v3d, vc4 always uses 1

int batch_steps();

#endif //  _V3DLIB_GRAVITY_DEFAULTS_H
