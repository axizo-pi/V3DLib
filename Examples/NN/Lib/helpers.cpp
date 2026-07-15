#include "helpers.h"

using namespace V3DLib;

namespace {

V3DLib::Settings _settings;

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


std::string vector_dump(Float::Array const &src, int size, int start_index, bool output_int) {
  assert(size <= (int) src.size());

  std::vector<std::string> ret;
  const int min_count = 5;

  auto out_val = [output_int] (float val) -> std::string {
    std::string buf;

    if (output_int) {
      buf << (int) val;
    } else {
      if (val == (int) val) {
        buf << (int) val;
      } else {
        buf << val;
      }
    }

    return buf;
  };


  auto out_buf = [min_count, &out_val] (float same_val, int same_count) -> std::string {
    std::string buf;

    if (same_count >= min_count) {
      buf << out_val(same_val) << " x " << same_count;
    } else {
      for (int i = 0; i < same_count; ++i) {
        buf << out_val(same_val);
      }
    }
    return buf;
  };

  auto vectorToString = [] (const std::vector<std::string>& vec, const std::string& delimiter) -> std::string {
    std::string result;
    for (const auto& str : vec) {
        if (!result.empty()) result += delimiter;
        result += str;
    }
    return result;
  };


  int   same_count = 0;
  float same_val   = src[start_index];

  for (int h = 0; h < size; ++h) {
    float val = src[start_index + h];

    if (val == same_val) {
      same_count++;
    } else {
      ret.push_back(out_buf(same_val, same_count));

      same_count = 1;
      same_val   = val;
    }
  }

  ret.push_back(out_buf(same_val, same_count));

  return vectorToString(ret, ", ");
}


Settings &settings() {
  return _settings;
}
