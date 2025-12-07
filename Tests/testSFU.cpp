#include "doctest.h"
#include <iostream>
#include <cmath>
#include "V3DLib.h"

using namespace V3DLib;

namespace {

float PI = 3.14159f;

void sfu(Float x, Float::Ptr r) {
  *r = 0.0f;                 r.inc();
  *r = 2.0f*x;               r.inc();  // Float mult
  *r = 2*x;                  r.inc();  // Int mult

  Float var = -2.0f;
  *r = var*x;                r.inc();  // Float mult from var

	// Prefixes to avoid conflicts with lib functions
  *r = V3DLib::exp(3.0f);    r.inc();
  *r = V3DLib::exp(x);       r.inc();

  *r = V3DLib::recip(x);     r.inc();
  *r = V3DLib::recipsqrt(x); r.inc();
  *r = V3DLib::log(x);       r.inc();
  *r = V3DLib::exp_e(1);     r.inc();
  *r = V3DLib::exp_e(x);
}


void check(float val, Float::Array &results, double precision) {
  REQUIRE(results[0]    == 0.0f);
  REQUIRE(results[16]   == 2*val);
  REQUIRE(results[16*2] == 2*val);
  REQUIRE(results[16*3] == -2*val);
  REQUIRE(abs(results[ 16*4] - 8.0)           <   precision);
  REQUIRE(abs(results[ 16*5] - exp2(val))     <   precision);  // Should be exact, but isn't
  REQUIRE(abs(results[ 16*6] - 1/val)         <   precision);
  REQUIRE(abs(results[ 16*7] - (1/sqrt(val))) <   precision);
  REQUIRE(abs(results[ 16*8] - log2(val))     <   precision);
  REQUIRE(abs(results[ 16*9] - exp(1))        < 2*precision);  // A bit less precise, is a calc
  REQUIRE(abs(results[16*10] - exp(val))     <  50*precision);
}

/*
std::string vector_dump(Float::Array const &src) {
	std::string buf;

  for (int h = 0; h < (int) src.size(); h += 16) {
    buf << src[h] << ", ";
  }

	return buf;
}
*/

}  // anon namespace


TEST_CASE("Test SFU functions [sfu]") {

  int N = 11;  // Number of results returned

  Float::Array results(16*N);

  auto k = compile(sfu);
  k.dump("SFU.txt");

  INFO("Running qpu");
  //
  // For vc4 one of the QPU SFU values are exact! Significant difference with int and emu.
  // Perhaps due to float precision, as opposed to double on cpu?
  //
  // v3d has same output as int and emu.
  //
  double precision = (Platform::run_vc4())?3e-4:1e-6;

  results.fill(0.0);

  k.load(1.0f, &results).run();
  //Log::warn << vector_dump(results);
  check(1.0f, results, precision);

  k.load(1.1f, &results).run();
  check(1.1f, results, precision);

  k.load(2.5f, &results).run();
  check(2.5f, results, precision);

  k.load(PI, &results).run();
  check(PI, results, precision);

// Following generates nanf - for 1/x
//  k.load(0.0f, &results).run();
//  check(0.0f, results, precision);
}
