#include "doctest.h"
#include "V3DLib.h"
#include "support/support.h"
#include "Support/Helpers.h"  // bit_diff()
#include <iostream>
#include <cmath>

using namespace V3DLib;

namespace {

float PI = 3.14159f;


/**
 * @brief Check if a sequence of floats have the same value
 */
bool same(float *in_ptr, int offset, int length = 16) {
  bool ret = true;
  float max_diff = 0.0f;
  float *ptr = in_ptr + offset;
  float val = *ptr;

  for (int i = 1; i < length; ++i) {
    ptr++;
    float in_val = *ptr;

    // Compare bit-size, ignore the least significant bit in the fraction
    if (bit_diff(val, in_val) <= 0) continue;
    
    float diff = (float) fabs(val - in_val);
    if (max_diff < diff) {
      max_diff = diff;
    }

    if (diff != 0.0f) {
      if (ret) { // Only show first
        cerr << "same(), index: " << i << " has diff: " << diff
             << ", val: " << val << ", in_val: " << in_val
             << ", diff: " << (val - in_val)
             << ", bit_diff:" << bit_diff(val, in_val);
      }
      ret = false;
    }
  }

  if (!ret) {
    cerr << "same(), offset: " << offset << ", max_diff: " << max_diff;
  }

  return ret;
}


void sfu_kernel(Float x, Float::Ptr r) {
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
  *r = V3DLib::exp_e(x);     r.inc();
}


/**
 * @brief Check results SFU kernel
 */
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
  REQUIRE(abs(results[16*10] - exp(val))      <  50*precision);
}


/**
 * @brief Test non-SFU library functions
 */
void lib_kernel(Float::Ptr in_ptr, Float::Ptr res_ptr, Int N_input) {
  Float::Ptr in = in_ptr;
  Float input;
  Float result;

  // Do vectors separately
	auto per_vector = [&] (void (*f)(Float &in, Float &out)) {
  	in = in_ptr;

		For (Int i = 0, i < N_input, i++)
	  	input = *in;
	  	f(input, result);
	  	*res_ptr = result;

	  	in.inc();
			res_ptr.inc();
		End
	};

  // Combine vectors
	auto combined_vectors = [&] (
		void (*f)(Float &in, Float &out),
		void (*f_final)(Float &in, Float &out)
	) {
  	in = in_ptr;
	  input = 0;

		For (Int i = 0, i < N_input, i++)
			Float in_val = *in;
	  	f(in_val, input);
	    in.inc();
		End

		f_final(input, result);
  	*res_ptr = result;
  	res_ptr.inc();
	};


  //
  // Test rotate_sum
  //
	per_vector(rotate_sum);
	combined_vectors(
		[] (Float &in, Float &out) { out += in; },
		rotate_sum
	);

  //
  // Test rotate_max
  //
	per_vector(rotate_max);
	combined_vectors(
		[] (Float &in, Float &out) { out = max(out, in); },
		rotate_max
	);
}

}  // anon namespace


TEST_CASE("Test SFU functions [sfu]") {

  int N = 11;  // Number of results returned

  Float::Array results(16*N);

  auto k = compile(sfu_kernel);
  //to_file("sfu_kernel.txt", k.dump());

  INFO("Running qpu");
  //
  // For vc4 one of the QPU SFU values are exact! Significant difference with int and emu.
  // Perhaps due to float precision, as opposed to double on cpu?
  //
  // v3d has same output as int and emu.
  //
  double precision = (Platform::run_vc4())?3e-4:1e-6;

  results.fill(0.0);

  INFO("Testing 1.0f");
  k.load(1.0f, &results).run();
  check(1.0f, results, precision);

  INFO("Testing 0.5f");
  k.load(2.5f, &results).run();
  //Log::warn << dump_array(results, 16, true);
  check(2.5f, results, precision);

  INFO("Testing 1.1f");
  k.load(1.1f, &results).run();
  check(1.1f, results, precision);

  INFO("Testing 2.5f");
  k.load(2.5f, &results).run();
  check(2.5f, results, precision);

  INFO("Testing PI");
  k.load(PI, &results).run();
  check(PI, results, precision);

// Following generates nanf - for 1/x
//  k.load(0.0f, &results).run();
//  check(0.0f, results, precision);
}


TEST_CASE("Test library functions [sfu][rotate]") {

  auto k = compile(lib_kernel);

	const int N_input = 3;

  Float::Array input(16*N_input);
  input.copyFrom({
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
   836, 397, 130, 574, 688, 964, 218, 432, 84, 820, 611, 108, 729, 15, 187, 266,
   250.48, 223.44,  716.1, 386.98, 436.13, 486, 336.65, 200.62, 144.04, 616.72, 575.89, 567.03, 371.21, 914.81, 148.14, 839.69
  });

  const int N = 2*(N_input + 1);  // Per operation result for separate vectors and 1 for combined vectors
  Float::Array result(16*N);

  k.load(&input, &result, N_input).run();

  //warn << dump_array(input, 16, false);
  //warn << dump_array(result, 16, false);

	//
	// Check that all elements of the result vectors are the same
	//
  auto r_ptr = result.ptr();
	for (int i = 0; i < N; ++i) {
	  REQUIRE(same(r_ptr, i*16, 16));
	}

	//
	// Check results - just checking the first element suffices
	//
	auto check_expected = [] (
		int N,
		int start_index,
		Float::Array &result,
		std::vector<float> const &expected,
		std::vector<int> const &vc4_bit_diff = {}
	) {
		for (int i = 0; i < N+ 1; i++) {
			int tmp_bit_diff = -1;

	    // Tiny differences can occur for vc4 (notably for rotate_sum)
	  	if (!vc4_bit_diff.empty() && Platform::run_vc4()) {
				tmp_bit_diff = vc4_bit_diff[i];
			}

	    REQUIRE(bit_diff(result[start_index + 16*i],  expected[i], tmp_bit_diff) == -1);
		}
	};

	// First four result vectors are sums
	std::vector<float> sum_expected = { 120.0f, 7059.0f, 7213.93f, 14392.93f};
	std::vector<int> vc4_bit_diff = { -1, -1, 1, 0};

	check_expected(N_input, 0, result, sum_expected, vc4_bit_diff);

	// Next 4 vectors are max values
	std::vector<float> max_expected = { 15.0f, 964.0f, 914.81f, 964.00f};

	check_expected(N_input, 64, result, max_expected);
}
