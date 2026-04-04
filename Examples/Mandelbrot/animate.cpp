#include "animate.h"
#include "Settings.h"

namespace animate {
namespace {

int animate_steps = 500;
int current = 0;

// Nice zoom-in range, used for testing
MandRange start;
//MandRange target(-2.0f, 0.75f, -1.0f, -0.25f);

// Source of examples: https://mandel.gart.nz/#/

// Julia Island
// can't get close enough
/*
const float centre_re = -1.768778833f;
const float centre_im =  0.001738996f;
const float diff      =  1e-14f;
MandRange target(centre_re - diff, centre_im - diff, centre_re + diff, centre_im + diff);
*/

// Seahorse Valley
/*
const float centre_re = -0.743517833f;
const float centre_im =  0.127094578f;
const float diff      =  2e-5f;
MandRange target(centre_re - diff, centre_im - diff, centre_re + diff, centre_im + diff);
*/

// Spirals
const float centre_re = -0.343806077f;
const float centre_im =  -0.61127804f;
const float diff      =  2e-6f;
MandRange target(centre_re - diff, centre_im - diff, centre_re + diff, centre_im + diff);


} // anon namespace

void init(MandRange const &in_start) {
	target.numStepsWidth  = in_start.numStepsWidth;
	target.numStepsHeight = in_start.numStepsHeight;

	start = in_start;
}


MandRange next() {
	MandRange ret = start;

	float factor = ((float) current)/((float) animate_steps);

  ret.topLeftReal     = start.topLeftReal     + factor*(target.topLeftReal     - start.topLeftReal);
  ret.topLeftIm       = start.topLeftIm       + factor*(target.topLeftIm       - start.topLeftIm);
  ret.bottomRightReal = start.bottomRightReal + factor*(target.bottomRightReal - start.bottomRightReal);
  ret.bottomRightIm   = start.bottomRightIm   + factor*(target.bottomRightIm   - start.bottomRightIm);

	current++;

	return ret;
}


bool done() {
	return current >= animate_steps;
}

} // namespace animate
