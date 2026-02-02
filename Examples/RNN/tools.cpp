#include "tools.h"
#include <ctime>
#include <cmath>

using namespace V3DLib;

namespace {

Settings _settings;

} // anon namespace

/**
 * Source: https://stackoverflow.com/questions/686353/random-float-number-generation
 */
float frand() {
  static bool did_init = false;
  if (!did_init) {
    srand (static_cast <unsigned> (time(0)));
    did_init = true;
  }
/*
  // Generate a number from 0.0 to 1.0, inclusive.
  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

  // Generate a number from 0.0 to some arbitrary float, X
  const float X = 1.0f;
  float r = static_cast <float> (rand()) / (static_cast <float> (((float) RAND_MAX)/X));
*/

  // Generate a number from some arbitrary LO to some arbitrary HI:
  const float LO = -1.0f;
  const float HI =  1.0f;
  float r = LO + static_cast <float> (rand()) /( static_cast <float> (((float) RAND_MAX)/(HI-LO)));

  return r;
}


std::string vector_dump(Float::Array const &src, int size, int start_index, bool output_int) {
	assert(size <= (int) src.size());
	std::string buf;

  for (int h = 0; h < size; ++h) {
		if (output_int) {
    	buf << (int) src[start_index + h] << ", ";
		} else {
    	buf << src[start_index + h] << ", ";
		}
  }

	return buf;
}

Settings &settings() { return _settings; }


namespace {

unsigned       s_seed         = 0;
unsigned const s_m            = 6012119;
unsigned       s_frrand_count = 0;

/**
 * Sample random numbers using a linear congruential generator.
 *
 * The goal is have exactly the same random number generator on ruby and C++.
 * Checked for the first million values.
 *
 * Source: https://predictivesciencelab.github.io/data-analytics-se/lecture07/hands-on-07.1.html
 */
unsigned lcg(unsigned x) {
  unsigned const a = 123456;
  unsigned const b = 978564;

  return (a * x + b) % s_m;
}

} // anon namespace


unsigned rrand() {
	s_seed = lcg(s_seed);
	return s_seed;
}


float frrand() {
	s_frrand_count++;
	unsigned val = rrand();
	return -1.0f + 2.0f*((float) val)/((float) s_m);
}


unsigned frrand_count() {
	return s_frrand_count;
}

