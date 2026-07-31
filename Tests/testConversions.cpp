#include <V3DLib.h>
#include "support/support.h"
#include "support/check.h"
#include "Support/Helpers.h"  // to_file()

using namespace V3DLib;

namespace {


///////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////

std::string dump_uint8_val(int val) {
  std::string ret;

  // Bitwise conversion to avoid issues with signs
  unsigned tmp = *((uint32_t *) &val);

  ret << (tmp & 0xff)         << ", "
      << ((tmp >>  8) & 0xff) << ", "
      << ((tmp >> 16) & 0xff) << ", "
      << ((tmp >> 24) & 0xff)
  ;

  return ret;
};


/**
 * @brief Display uint8 values
*/
MAYBE_UNUSED std::string dump_uint8(Int::Array const &values) {
  std::string ret;
  //ret << "size: " << values.size() << " <";

  for (int i = 0; i < (int) values.size(); i++) {
    ret << dump_uint8_val(values[i]) << ", ";
  }

  //ret << ">";
  return ret;
}  


///////////////////////////////////////////////////
// Kernels
///////////////////////////////////////////////////

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


/**
 * @partial to make 4 consecutive copies of the first 4 elements of a vector
 *
 * E.g.:
 *
 * input : <a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p>
 * result: <a,a,a,a,b,b,b,b,c,c,c,c,d,d,d,d>
 */
void duplicate_first_4_elements_partial(Int &res, Int &val) {
  Int tmp;
  Int tmp2;

  tmp = val;
  Where (index() != 0)
    tmp = 0;
  End
  rotate_sum(tmp, tmp2);
    
  Where (index() < 4)
    res = tmp2;
  End

  tmp = val;
  Where (index() != 1)
    tmp = 0;
  End
  rotate_sum(tmp, tmp2);
    
  Where (index() >= 4 && index() < 8)
    res = tmp2;
  End

  tmp = val;
  Where (index() != 2)
    tmp = 0;
  End
  rotate_sum(tmp, tmp2);
    
  Where (index() >= 8 && index() < 12)
    res = tmp2;
  End

  tmp = val;
  Where (index() != 3)
    tmp = 0;
  End
  rotate_sum(tmp, tmp2);
    
  Where (index() >= 12)
    res = tmp2;
  End
}


/**
 * @brief Convert incoming uint8 values to Int.
 *
 * Only first 4 words are handled, to fill up a 16-vector.
 * Pointer increment should be adjusted accordingly.
 */
void uint8_to_Int_partial(Int &res, Int &val) {
  duplicate_first_4_elements_partial(res, val);

  Where (index() % 4 == 0)
    res = res & 0xff;
  End

  Where (index() % 4 == 1)
    res = (res >> 8) & 0xff;
  End

  Where (index() % 4 == 2)
    res = (res >> 16) & 0xff;
  End

  Where (index() % 4 == 3)
    res = (res >> 24) & 0xff;
  End
}


/**
 * @brief Convert incoming Int values to uint8.
 *
 * The result is 4 words long. 
 * Overflow and sign change are negated and therefore ignored.
 *
 * TODO: incorporate this into unit test.
 */
void Int_to_uint8_partial(Int &res, Int &val) {
  Int tmp = val;
  tmp = tmp & 0xff;   // Mask to avoid issues with overflow and sign

  Int tmp1;
  Int tmp2;
  Int tmp3;
  tmp1 = rotate(tmp, 15);
  tmp2 = rotate(tmp, 14);
  tmp3 = rotate(tmp, 13);

  // Move bytes to correct position
  // tmp as is
  tmp1 = (tmp1 <<  8);
  tmp2 = (tmp2 << 16);
  tmp3 = (tmp3 << 24);

  tmp = tmp | tmp1 | tmp2 | tmp3;

  //
  // At this point, vector elements 0,4,8,12 are correct. The rest is junk.
  // Move these to first four elements.
  //
  tmp1 = tmp;
  tmp1 = rotate(tmp1, 13);
  Where (index() == 1)
    tmp = tmp1;
  End

  tmp1 = rotate(tmp1, 13);
  Where (index() == 2)
    tmp = tmp1;
  End

  tmp1 = rotate(tmp1, 13);
  Where (index() == 3)
    tmp = tmp1;
  End

/*    
  // Zap the trailing elements - doesn't do much
  Where (index() > 3)
    tmp = 0;
  End
*/
  res = tmp;
}


/**
 * @brief Kernel to test conversion uint8 <-> Int (both ways)
 *
 * Lesson learnt and to remember: ARM endianess is LSB.
 *
 * @param res_int Array to store internal Int values, to check them
 * @param values  Incoming values; every byte is a uint8 value
 * @param N       Number of incoming 4-byte values
 */
void uint8_to_Int_kernel(Int::Ptr res_int, Int::Ptr res, Int::Ptr values, Int N) {
/*  
  res -= index();
  Where (index() < 4)
    res += index();
  Else
    res += 4;
  End
*/  
/*  
  Where (index() >4)
    res -= index();
    res += 4;
  End
*/  

  For (Int n = 0, n < N, n += 4)
    Int val = *values;
    Int internal = 0;

    uint8_to_Int_partial(internal, val);

    // At this point, res contains the next 16 uint8 values

    *res_int = internal;  // Store internal Int values, for testing

    // Do some kind of operation here on internal
    internal *= 2;

    // Store results as uint8
    Int result;
    Int_to_uint8_partial(result, internal);
    *res = result;

    values += 4;   comment("Do pointer increments");
    res    += 4;
    res_int.inc(); comment("Increment res_int");
  End
}  

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

/*
  std::string buf;
  buf << "result unsigned: ";
  for (int i = 0; i < 16*Blocks; ++i) {
    buf << ((unsigned) result[i]) << ", ";
  }
  warn << buf;
*/

  float Precision = 0.0f;
  if (Platform::compiling_for_vc4()) {
    Precision = 256.0f;  // vc4 not precise
  }

  // Float result is compared to unsigned values
  check_vector(result, 0, expected, Precision);
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


TEST_CASE("Test conversion uint8 -> float [convert]") {

  auto uint8_to_int = [] (std::vector<uint8_t> const &values, int index) -> int {
    int ret = 0;
    int n = index;

    unsigned val0 = values[n];
    unsigned val1 = values[n + 1];
    unsigned val2 = values[n + 2];
    unsigned val3 = values[n + 3];
    unsigned tmp = (val3 << 24) + (val2 << 16) + (val1 << 8) + val0;

    // Bitwise conversion to avoid issues with signs
    ret  = *((int *) & tmp);
    //warn << "ret: " << hex << ret;

    return ret;
  };

  std::vector<uint8_t> values_8 = {
    0, 1, 2, 3,
    4, 5, 6, 7,
    8, 9, 10, 11,
    12, 13, 14, 15,

    // Test limits for overflow
    0,   127,   1,  64,
    0,   128,   1, 255,

    // Arbitrary values
    2,   100,   4, 100,
    6,   100,   8, 100,
    16,  100,  32, 100,
    64,  100, 128, 100,
    255, 100,  96, 100,
  
    // End marker  
    0xff, 0xff, 0xff, 0xff
  };

  REQUIRE(values_8.size() % 4 == 0);

  // Default values for qpu are 4 byte words.
  // There are 4 uint8's in an Int
  Int::Array values(16);
  values.fill(0);

  for (int i = 0; i < (int) values_8.size()/4; ++i) {
    values[i] = uint8_to_int(values_8, 4*i);
  }

  //warn << "values         :  " << dump_uint8(values);

  //
  // Perform the actual test
  //
  Int::Array result_internal(4*16);
  result_internal.fill(-1);

  Int::Array result(16 + 1);
  result.fill(-2);

  auto k = compile(uint8_to_Int_kernel);
  //to_file("uint8_to_Int_kernel.txt", k.dump());
  k.load(&result_internal, &result, &values, (int) values.size()).run();

  //warn << "result_internal: " << dump_array(result_internal);
  //warn << "result         : " << dump_uint8(result);

  // Check internal values
  for (int i = 0; i < (int) values_8.size(); ++i) {
    INFO("Check internal i = " << i);
    REQUIRE(values_8[i] == result_internal[i]);
  }

  // Check result
  uint8_t *result_ptr = (uint8_t *) result.ptr();

  for (int i = 0; i < (int) values_8.size(); ++i) {
    INFO("Check result i = " << i);
    uint8_t val = 2* values_8[i];   // too big values will overflow
    REQUIRE(val == result_ptr[i]);
  }
}
