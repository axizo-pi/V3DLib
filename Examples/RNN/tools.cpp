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
