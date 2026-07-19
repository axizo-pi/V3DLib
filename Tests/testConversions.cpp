#include <V3DLib.h>
#include "support/support.h"
#include "support/check.h"

using namespace V3DLib;

namespace {

void signed_to_float_kernel(Float::Ptr result, Int::Ptr input, Int Blocks) {
  For (Int i = 0, i < Blocks, i++)
    Int   n = *input;
    Float r = toFloat(n);
    *result = r;

    input.inc();
    result.inc();
  End
}


void unsigned_to_float_kernel(Float::Ptr result, Int::Ptr input, Int Blocks) {
  For (Int i = 0, i < Blocks, i++)
    Int   n = *input;
    Float r = UnsignedtoFloat(n);
    *result = r;

    input.inc();
    result.inc();
  End
}


void float_fields_kernel(
	Float::Ptr values,
	Int::Ptr   out_sign,
	Int::Ptr   out_exponent,
	Int::Ptr   out_significand,
	Int        Blocks
) {
	Int sign;
	Int exponent;
	Int significand;

	For (Int i =0, i < Blocks, i++)
		Float val = *values;

		functions::float_fields(val, sign, exponent, significand);

		*out_sign        = sign;
		*out_exponent    = exponent;
		*out_significand = significand;

		values.inc();
		out_sign.inc();
		out_exponent.inc();
		out_significand.inc();
	End
}


const int Blocks = 4;

std::vector<int> values = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 , 15,
  0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14 , -15,
  22951, 82873, 83242, 90738, 91688, 23135, 45929, 54547, 28837, 74328, 80697, 48614, 82437, 49339, 3386, 70684,
  -22951, -82873, -83242, -90738, -91688, -23135, -45929, -54547, -28837, -74328, -80697, -48614, -82437, -49339, -3386, -70684
};

} // anon namespace


TEST_CASE("Test conversion of signed to float [convert]") {
  Int::Array input(16*Blocks);
  input.copyFrom(values);
  //warn << "input: " << dump_array(input);

  Float::Array result(16*Blocks);

  auto k = compile(signed_to_float_kernel);
  k.load(&result, &input, Blocks).run();
  //warn << "result: " << dump_array(result);

  // Float result is compared to int values
  check_vector(result, 0, values);
}


TEST_CASE("Test conversion of unsigned to float [convert]") {
  Int::Array input(16*Blocks);
  input.copyFrom(values);
  //warn << "input: " << dump_array(input);

  std::vector<unsigned> expected(16*Blocks);
  REQUIRE(expected.size() == values.size());

  for (int i = 0; i < (int) values.size(); ++i) {
    expected[i] = *((unsigned *) &values[i]);
  }
  //warn << "expected: " << dump_array(expected);

  Float::Array result(16*Blocks);

  auto k = compile(unsigned_to_float_kernel);
  k.load(&result, &input, Blocks).run();
  //warn << "result: " << dump_array(result);

  // Float result is compared to unsigned values
  check_vector(result, 0, expected);
}


TEST_CASE("Test working of float_fields [convert]") {
	//warn << "Doing float_fields";

	const int Blocks = 1;

	//
	// Notable single-precision cases.
	// Source: https://en.wikipedia.org/wiki/Single-precision_floating-point_format
	//
	std::vector<unsigned> values_u = {
		0x00000001, // 1.40129846430e-45f (smallest positive subnormal number)
		0x007fffff, // 1.1754942107 × 10−38 (largest subnormal number)
		0x00800000, // 1.1754943508 × 10−38 (smallest positive normal number)
		0x7f7fffff, // 3.4028234664 × 1038 (largest normal number)
		0x3f7fffff, // 0.999999940395355225 (largest number less than one)
		0x3f800000, // 1 (one)
		0x3f800001, // 1.00000011920928955 (smallest number larger than one)
		0xc0000000, // −2
		0x00000000, // 0
		0x80000000, // −0
		0x7f800000, // infinity
		0xff800000, // −infinity
		0x3eaaaaab, // 0.333333343267440796, 1/3
		0x40490fdb, // 3.14159274101257324,  pi
		0x7fc00001, // qNaN (on x86 and ARM processors); removed sign for test (original 0xf...)
		0xff800001  // sNaN (on x86 and ARM processors)
	};

	Float::Array values(16*Blocks);

	// Reinterpret as floats
  for (int i = 0; i < (int) values_u.size(); ++i) {
    values[i] = *((float *) &values_u[i]);
  }
/*
	// Add same values negated
  for (int i = 0; i < (int) values_u.size(); ++i) {
    unsigned tmp = values_u[i] + (1 << 31);
    values[i + 16] = *((float *) &tmp);
  }
*/	
  //warn << "values: " << dump_array(values);

	Int::Array sign(16*Blocks);
	Int::Array exponent(16*Blocks);
	Int::Array significand(16*Blocks);

  auto k = compile(float_fields_kernel);
  k.load(&values, &sign, &exponent, &significand, Blocks).run();

  //warn << "sign       : " << dump_array(sign);
  //warn << "exponent   : " << dump_array(exponent);
  //warn << "significand: " << dump_array(significand);

	// Reconstruct values to check
	for (int i = 0; i < 16; ++i) {
		INFO("index: " << i);
		unsigned val = (sign[i] << 31) + ((exponent[i] + 127) << 23) + significand[i];
		REQUIRE(values_u[i] == val);
	}
}
