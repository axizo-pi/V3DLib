#ifndef _V3DLIB_GRAVITY_DEFAULTS_H
#define _V3DLIB_GRAVITY_DEFAULTS_H

float const IMG_CONVERSION_FACTOR = 2.5e13f;

const int N_PLANETS   = 9;                       // Also includes the sun, pluto missing

double const dt    = 86400;                      // 1 day
extern double t_end;                             // End time for calculation. Default: 250 years.
double const BIG_G = 6.67e-11;                   // gravitational constant, (m^3⋅kg^−1⋅s^−2)

int const BATCH_STEPS = 2;                       // for v3d, vc4 always uses 1

void set_num_years(int val);
int batch_steps();

#endif //  _V3DLIB_GRAVITY_DEFAULTS_H
