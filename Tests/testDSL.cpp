#include <V3DLib.h>
#include "support/support.h"
#include "support/check.h"
#include "Support/Helpers.h"  // to_file()
#include "v3d/v3d.h"
#include "LibSettings.h"
#include <cmath>

using namespace V3DLib;
using namespace std;

namespace {

//=============================================================================
// Helper Functions
//=============================================================================

template< typename T1, typename T2>
float calc_max_diff(T1 &arr1, T2 &arr2, int size) { 
  float max_diff = 0.0f;

  for (int x = 0; x < size; ++x) {
    float tmp = std::abs(arr1[x] - arr2[x]);
    if (tmp > max_diff) max_diff = tmp;
  }

  return max_diff;
}


void check(Int::Array const &result, std::vector<int> const &expected, std::string const &label) {
    for (int i = 0; i < (int) expected.size(); i++) {
      INFO("label: " << label << ", row: " << (i/16) << ", index: " << (i % 16));
      REQUIRE(result[i] == expected[i]);
    }
  };


//=============================================================================
// Kernel definitions
//=============================================================================

void out(Int &res, Int::Ptr &result) {
  *result = res;
  result = result + 16;
}


void out(Float &res, Float::Ptr &result) {
  *result = res;
  result = result + 16;
}


void test(Cond cond, Int::Ptr &result) {
  Int res = -1;  // temp variable for result of condition, -1 is unexpected value

  If (cond)
    res = 1;
  Else
    res = 0;
  End

  out(res, result);
}


/**
 * @brief Overload for BoolExpr
 *
 * TODO: Why is distinction BoolExpr <-> Cond necessary? Almost the same
 */
void test(BoolExpr cond, Int::Ptr &result) {
  Int res = -1;  // temp variable for result of condition, -1 is unexpected value

  If (cond)
    res = 1;
  Else
    res = 0;
  End

  out(res, result);
}


void kernel_specific_instructions(Int::Ptr result) {
  Int a = index();
  Int b = a ^ 1;
  out(b, result);

  b = a  + 1;
  b *= 3;
  out(b, result);
}


void kernel_specific_float_instructions(Float::Ptr result) {
  Float a = toFloat(index() + 1);
  //Float b = a / b; - seq fault! TODO detect
  Float b = 1 / a;
  out(b, result);

  b = a;
  b *= 3.1f;
  out(b, result);
}



/**
 * @brief Kernel for testing If and When
 */
void kernelIfWhen(Int::Ptr result) {
  Int outIndex = index();
  Int a = index();

  // any
  test(any(a <   0), result);

  comment("Start any(a < 8) below");
  test(any(a <   8), result);
  comment("Done any(a < 8)");

  test(any(a <=  0), result);  // Boundary check
  test(any(a >= 15), result);  // Boundary check
  test(any(a <  32), result);
  test(any(a >  32), result);

  // all
  test(all(a <   0), result);
  test(all(a <   8), result);
  test(all(a <=  0), result);  // Boundary check
  test(all(a >= 15), result);  // Boundary check
  test(all(a <  32), result);
  test(all(a >  32), result);

  // Just If - should be same as any
  test((a <   0), result);
  test((a <   8), result);
  test((a <=  0), result);     // Boundary check
  test((a >= 15), result);     // Boundary check
  test((a <  32), result);
  test((a >  32), result);

  // When
  Int res = -1;  // temp variable for result of condition, -1 is unexpected value
  Where (a < 0) res = 1; Else res = 0; End
  out(res, result);

  res = -1;
  Where (a <= 0) res = 1; Else res = 0; End  // Boundary check
  out(res, result);

  res = -1;
  Where (a >= 15) res = 1; Else res = 0; End  // Boundary check
  out(res, result);

  res = -1;
  Where (a < 8) res = 1; Else res = 0; End
  out(res, result);

  res = -1;
  Where (a >= 8) res = 1; Else res = 0; End
  out(res, result);

  res = -1;
  Where (a < 32) res = 1; Else res = 0; End
  out(res, result);

  res = -1;
  Where (a > 32) res = 1; Else res = 0; End
  out(res, result);
}


void check_conditionals(Int::Array &result, int N) {
  vector<int> allZeroes = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  vector<int> allOnes   = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

  auto assertResult = [N] ( Int::Array &result, int index, std::vector<int> const &expected) {
    INFO("index: " << index);
    REQUIRE(result.size() == (unsigned) N*16);
    check_vector(result, index, expected);
  };

  // any
  assertResult(result,  0, allZeroes);
  assertResult(result,  1, allOnes);
  assertResult(result,  2, allOnes);
  assertResult(result,  3, allOnes);
  assertResult(result,  4, allOnes);
  assertResult(result,  5, allZeroes);

  // all
  assertResult(result,  6, allZeroes);
  assertResult(result,  7, allZeroes);
  assertResult(result,  8, allZeroes);
  assertResult(result,  9, allZeroes);
  assertResult(result, 10, allOnes);
  assertResult(result, 11, allZeroes);

  // Just If - should be same as any
  assertResult(result, 12, allZeroes);
  assertResult(result, 13, allOnes);
  assertResult(result, 14, allOnes);
  assertResult(result, 15, allOnes);
  assertResult(result, 16, allOnes);
  assertResult(result, 17, allZeroes);

  // where
  assertResult(result, 18, allZeroes);
  assertResult(result, 19, {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0});
  assertResult(result, 20, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1});
  assertResult(result, 21, {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0});
  assertResult(result, 22, {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1});
  assertResult(result, 23, allOnes);
  assertResult(result, 24, allZeroes);
}


void complex_kernel(Complex::Ptr input, Complex::Ptr result) {
  Complex a = *input;
  Complex b = a*a;
  *result = b;
}


void floor_kernel_vc4(Float::Ptr result, Float::Ptr input, Int numValues) {
  For (Int n = 0, n < numValues, n += 16)
    *result = functions::ffloor_vc4(*input);

    result.inc();
    input.inc();
  End
}


void floor_kernel(Float::Ptr result, Float::Ptr input, Int numValues) {
  For (Int n = 0, n < numValues, n += 16)
    *result = functions::ffloor(*input);

    result.inc();
    input.inc();
  End
}


void fabs_kernel(Float::Ptr result, Float::Ptr input, Int numValues) {
  For (Int n = 0, n < numValues, n += 16)
    *result = functions::fabs(*input);

    result.inc();
    input.inc();
  End
}


/**
 * This should try out all the possible ways of reading and writing
 * main memory.
 */
template<typename T, typename Ptr>
void offsets_kernel(Ptr result, Ptr src) {
  Int a = index();
  *result = a;
  result.inc();           comment("Kernel inc 0");

  T val = *src;
  *result = val;          comment("Kernel inc 1");


  T val2 = *(src + 32);
  *(result + 16) = val2;  comment("Kernel inc 3");

  val2 = src[32];
  result[32] = val2;      comment("Kernel inc 4");

  src.inc();
  result.inc();
  result.inc();
  result.inc();


  val = *src;
  *result = val;

  result.inc();

  gather(src);  comment("Start gather test");
  receive(a);
  *result = a;
}

}  // namespace


//=============================================================================
// Unit tests
//=============================================================================

TEST_CASE("Test correct working DSL [dsl][instr]") {
  REQUIRE(::v3d::open());

  SUBCASE("Test specific int instructions") {
    int const NUM = 2;
    vector<int> expected = {
      1, 0, 3,  2,  5,  4,  7,  6,  9,  8, 11, 10, 13, 12, 15, 14,
      3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48
    };

    Int::Array result(16*NUM);
    result.fill(-2);  // Initialize to unexpected value

    auto k = compile(kernel_specific_instructions);
    k.load(&result).run();
    check_vector(result, 0, expected);
  }


  SUBCASE("Test specific float instructions") {
    const int NUM = 2;

    vector<float> expected;
     expected.resize(16*NUM); 

    for (int i = 0; i < 16; ++i) {
      expected[i] = 1.0f/(1.0f + ((float) i));
    }
    for (int i = 16; i < 32; ++i) {
      expected[i] = ((float) (i - 16) + 1) * 3.1f;
    }

    Float::Array result(16*NUM);
    result.fill(-2);  // Initialize to unexpected value

    auto k = compile(kernel_specific_float_instructions);
    k.load(&result).run();
    //Log::warn << "result: " << result.dump();

    float precision = 0;
    if (Platform::run_vc4()) {
      precision = 0.5e-4f;
    }

    check_vector(result, 0, expected, precision);
  }
}


TEST_CASE("Test Conditionals [dsl][cond]") {
  //
  // Test all variations of If and When
  //
  SUBCASE("Conditionals work as expected") {
    int const N = 25;  // Number of expected result vectors

    auto k = compile(kernelIfWhen);

    Int::Array result(16*N);

    // Reset result array to unexpected values
    auto reset = [&result] () {
      result.fill(-2);
    };

    //
    // Run kernel in the three different run modes
    //
    INFO("Checking conditionals");
    reset();
    k.load(&result).run();
    check_conditionals(result, N);
  }
}


TEST_CASE("Test construction of composed types in DSL [dsl][complex]") {
  SUBCASE("Test Complex composed type") {
    const int N = 1;  // Number Complex items in vectors

    auto k = compile(complex_kernel);

    // Allocate and array for input and result values
    Complex::Array input(16*N);
    input.fill({0,0});
    input[0] = complex(1, 0);
    input[1] = complex(0, 1);
    input[2] = complex(1, 1);

    Complex::Array result(16*N);

    k.load(&input, &result).run();

    REQUIRE(result[0] ==  complex(1, 0));
    REQUIRE(result[1] ==  complex(-1, 0));
    REQUIRE(result[2] ==  complex(0, 2));
  }
}


//-----------------------------------------------------------------------------
// Tests for specific DSL operations.
//-----------------------------------------------------------------------------

namespace {

void int_ops_kernel(Int::Ptr result) {
  using namespace V3DLib::functions;

  //
  // NEVER FORGET:
  //
  // Previous definition:
  //
  //    auto store = [&result] (IntExpr const &val) {
  //
  // This resulted in the passed Int var to be converted to IntExpr,
  // and then back to Int, creating a useless interim variable in the source lang
  //
  auto store = [&result] (Int const &val) {
    comment("store starts next"); 
    *result = val;
    result.inc();
  };

  comment("add 3");
  Int a = index();
  a += 3;
  store(a);

  comment("sub 11");
  a -= 11;
  store(a);

  store(abs(index() - 8));
  store(two_complement(index() - 8));       // 2's complement, library call

  Int b = topmost_bit(1 << (index() + 3));
  store(b);

  b = -256;
  store(b);

  comment("First division test starts next");
  store(16*16/index());

  comment("Integer division by float");
  store(integer_division_f(16*16, index()));

  comment("First usage -index() starts next");
  store((-16*16)/(-index()));

  store((-16*16)/index());
  store(16*16/(-index()));
  store(17*index()/11);
}


/**
 * This tests the various ways of converting floats for small imm's
 */
void float_ops_kernel(Float::Ptr result) {
  auto store = [&result] (Float const &val) {
    *result = val;
    result.inc();
  };

  Float ndx = toFloat(index());
  Float a;
  a = ndx + 3.0f;
  a += 0.25f;
  store(a);

  a = ndx*47.0f;
  store(a);

  a = ndx + (-0.25f);
  store(a);

  //
  // NOTE; slight differences in results between scalar and QPU
  //

  a = ndx*1.1215f;
  store(a);

  a = -3.27f*ndx + 1.0f;  // without add, actually literally returns '-0'
  store(a);
}


void nested_for_kernel(Int::Ptr result) {
  int const COUNT = 3;
  Int x = 0;

  For (Int n = 0, n < COUNT, n++)
    For (Int m = 0, m < COUNT, m++)
      x += 1;

      Where ((index() & 0x1) == 1)
        x += 1;
      End

      If ((m & 0x1) == 1)
        x += 1;
      End
    End

    x += 2;
  End

  *result = x;
}


template<typename T, typename Ptr>
void rot_kernel(Ptr result, Ptr a) {
  T val = *a;
  T val2 = *a;

  val2 = rotate(val, 1);
  *result = val2; result.inc();

  val2 += rotate(val, 1);
  *result = val2; result.inc();

  rotate_sum(val, val2);
  *result = val2; result.inc();

  T val3 = val;
  set_at(val3, 0, val2);
  *result = val3;
}

} // anon namespace


TEST_CASE("Test specific operations in DSL [dsl][ops]") {
  SUBCASE("Test integer operations") {
    int const N = 12;  // Number of expected results

    auto k = compile(int_ops_kernel);
    //to_file("int_ops_kernel.txt", k.dump());

    Int::Array result(16*N);
    result.fill(-1);

    k.load(&result);
    k.run();

    vector<vector<int>> expected = {
      {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18},                    // +=
      {-8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7},                     // -=
      {8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7},                             // abs
      {8, 7, 6, 5, 4, 3, 2, 1, 0, -1, -2, -3, -4, -5, -6, -7},                      // 2-s complement
      {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18},                    // topmost_bit
      {-256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256, -256}, // b = -256 

      //
      // integer division
      //

      // First value is 'infinity'
      {2147483647, 256, 128, 85, 64, 51, 42, 36, 32, 28, 25, 23, 21, 19, 18, 17},

      // integer division by float
      {2147483647, 256, 128, 85, 64, 51, 42, 36, 32, 28, 25, 23, 21, 19, 18, 17},

      {-2147483647, 256, 128, 85, 64, 51, 42, 36, 32, 28, 25, 23, 21, 19, 18, 17},  // NB -0 == 0
      {-2147483647, -256, -128, -85, -64, -51, -42, -36, -32, -28, -25, -23, -21, -19, -18, -17},
      {2147483647, -256, -128, -85, -64, -51, -42, -36, -32, -28, -25, -23, -21, -19, -18, -17},
      {0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 15, 17, 18, 20, 21, 23}
    };

    check_vectors(result, expected);
  }


  SUBCASE("Test float operations") {
    int const N = 5;  // Number of expected results

    auto k = compile(float_ops_kernel);
    //to_file("float_ops_kernel.txt", k.dump());

    Float::Array results(16*N);

    k.load(&results).run();

    vector<vector<float>> expected = {
      { 3.25,  4.25,  5.25,  6.25,  7.25,  8.25,  9.25, 10.25, 11.25, 12.25, 13.25, 14.25, 15.25, 16.25, 17.25, 18.25},
      { 0, 47, 94, 141, 188, 235, 282, 329, 376, 423, 470, 517, 564, 611, 658, 705},
      { -0.25, 0.75, 1.75, 2.75, 3.75, 4.75, 5.75, 6.75, 7.75, 8.75, 9.75, 10.75, 11.75, 12.75, 13.75, 14.75},
      { 0, 1.1215, 2.243, 3.3645, 4.486, 5.6075, 6.729, 7.8505, 8.972, 10.0935, 11.215, 12.3365, 13.458, 14.5795, 15.701, 16.8225},
      { 1, -2.27, -5.54, -8.81, -12.08, -15.35, -18.62, -21.89, -25.16, -28.43, -31.7, -34.97, -38.24, -41.51, -44.78, -48.05},
    };

    check_vectors(results, expected, 1e-4f);
  }
}


TEST_CASE("Test For-loops [dsl][for]") {
  Platform::use_main_memory(true);

  SUBCASE("Test nested For-loops") {
    auto k = compile(nested_for_kernel);

    Int::Array result(16);
    k.load(&result).run();

    vector<int> expected = {18, 27, 18, 27, 18, 27, 18, 27, 18, 27, 18, 27, 18, 27, 18, 27};
    check_vector(result, 0, expected);
  }

  Platform::use_main_memory(false);
} 


/**
 * This tests stuff that went wrong at some point.
 */
TEST_CASE("Test rotate on emulator [emu][rotate]") {
  Platform::use_main_memory(true);
  int const N = 4;

  Int::Array a(16);
  Int::Array result1(N*16);
  result1.fill(-1);
  Int::Array result2(N*16);
  result2.fill(-1);

  auto reset = [&a] () {
    for (int i = 0; i < (int) a.size(); i++) {
      a[i] = (i + 1);
      //a[i] = (float) (i + 1);
    }
  };

  auto k = compile(rot_kernel<Int, Int::Ptr>);
  k.load(&result1, &a);

  // Interpreter works fine, used here to compare emulator output
  reset();
  k.interpret();

  std::cout << "\n";

  reset();
  k.load(&result2, &a);
  k.emu();

  REQUIRE(result1 == result2);

  Platform::use_main_memory(false);
}


/**
 * Created in order to test init uniforms pointers with index() for vc4
 */
TEST_CASE("Initialization with index() on uniform pointers should work as expected [dsl][offsets]") {
  int const N = 6;

  Int::Array a(3*16);
  Int::Array result(N*16);

  std::vector<int> expected = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};

  REQUIRE(expected.size() == N*16);
  REQUIRE(result.size() == expected.size());

  auto reset = [&a, &result] () {
    // Not necessary at this point, a does not change
    for (int i = 0; i < (int) a.size(); i++) {
      a[i] = (i + 1);
    }

    result.fill(-1);
  };


  SUBCASE("Test with TMU") {
    auto k = compile(offsets_kernel<Int, Int::Ptr>);
    reset();
    k.load(&result, &a).run();
    check(result, expected, "tmu qpu");
  }


  SUBCASE("Test with DMA") {
    LibSettings::use_tmu_for_load(false);

    auto k = compile(offsets_kernel<Int, Int::Ptr>);
    k.load(&result, &a).run();
    check(result, expected, "dma qpu");

    LibSettings::use_tmu_for_load(true);
  }
}


TEST_CASE("Test functions [dsl][func]") {
  REQUIRE(::v3d::open());

  int const NumValues       = 15;
  int const SharedArraySize = (NumValues/16 +1)*16;

  std::vector<float> input = {
     1.0f,
     1.3f,
    -1.0f,
    -1.3f,
     0.9f,
    -0.9f,
     1.0e-32f,
    -1.0e-32f,
     1.1e38f,
    -1.1e38f,
    // -1.1e-38f, // On v3d, this works as expected. On vc4, this registers as 0.0, not negative
    -1.1e-36f,    // Using this value instead
     7.0f,
     7.1f,
    //-7.0f,      // scalar floor handles this fine, qpu  makes this 08.0f
                  // This looks like a conversion issue (string -> binary float)
    -7.000001f,   // Using this instead
    -7.1f
  };
  REQUIRE(input.size() == NumValues);

  Float::Array input_qpu(SharedArraySize);
  input_qpu.copyFrom(input);

  SUBCASE("Test ffloor()") {
    INFO("Doing ffloor on qpu");
    float expected[NumValues];
    for (int n = 0; n < NumValues; ++n) {
      expected[n] = (float) floor(input[n]);
    }

    Float::Array result(SharedArraySize);

    auto check = [&]() {
      for (int n = 0; n < NumValues; ++n) {
        INFO("input   : " << dump_array(input_qpu));
        INFO("expected: " << dump_array(expected, NumValues));
        INFO("result  : " << dump_array(result));
        INFO("n: " << n);
        REQUIRE(expected[n] == result[n]);
      }
    };

    {
      result.fill(-1.0f);
      auto k = compile(floor_kernel_vc4);
      k.load(&result, &input_qpu, NumValues).run();
      check();
    }

    {
      result.fill(-1.0f);
      auto k = compile(floor_kernel);
      k.load(&result, &input_qpu, NumValues).run();
      check();
    }

    {
      if (!Platform::compiling_for_vc4()) {
        result.fill(-1.0f);

        BaseSettings settings;
        INFO("Doing ffloor on interpreter");
        settings.run_type = Interpreter;
        //INFO("Doing ffloor on emulator");
        //settings.run_type = Emulator;  // TODO: fails on certain values

        auto k = compile(floor_kernel, settings);
        k.load(&result, &input_qpu, NumValues).run();

        check();
      }
    }    
  }


  SUBCASE("Test fabs()") {
    float results_scalar[NumValues];
    for (int n = 0; n < NumValues; ++n) {
     results_scalar[n] = (float) abs(input[n]);
    }

    Float::Array results_qpu(SharedArraySize);
    results_qpu.fill(-1.0f);

    auto k = compile(fabs_kernel);
    k.load(&results_qpu, &input_qpu, NumValues).run();

    for (int n = 0; n < NumValues; ++n) {
      INFO("results_scalar: " << dump_array(results_scalar, NumValues));
      INFO("results_qpu   : " << dump_array(results_qpu));
      INFO("n: " << n);
      REQUIRE(results_scalar[n] == results_qpu[n]);
    }
  }
}


//=============================================================================
// Test Issues
//
// Test stuff which has been seen to go wrong.
//=============================================================================

namespace {

void issues_kernel(Int::Ptr result, Int::Ptr src) {
  Int a = 0;       comment("Start check 'If (a != b)' same as 'If (any(a !=b))'");
  Int c = 0;

  For (Int b = 0, b < 2, b++)
    // Generation of this and following If should be identical - visual check
    If (a != b)
      c = 1;
    End

    *result = c; result.inc();

    c = 0;

    If (any(a != b))
     c = 1;
    End

    *result = c; result.inc();
  End

  Int dummy = 0;   comment("Start ptr offset check");
  *result = 4*(index() + 16*me());
  result.inc();

  *result = *src;  comment("Check *dst = *src"); 
}

#pragma GCC diagnostic push                             // save the actual diag context
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // disable maybe warnings

//
// Following should all generate errors during compile
//
void init_self_1_kernel() { Int x = x; }
void init_self_2_kernel() { Float x = x; }
void init_self_3_kernel() { Complex y = y; }

#pragma GCC diagnostic pop     

}  // anon namespace


TEST_CASE("Test issues [dsl][issues]") {
  Platform::use_main_memory(true);

  SUBCASE("Verify issues") {
    int const N = 6;

    auto k = compile(issues_kernel);

    Int::Array input(16);
    input.fill(7);

    Int::Array result(16*N);
    result.fill(-1);

    k.load(&result, &input).run();

    check_vector(result, 0, 0);
    check_vector(result, 1, 0);
    check_vector(result, 2, 1);
    check_vector(result, 3, 1);

    std::vector<int> expected = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60};
    check_vector(result, 4, expected);
    check_vector(result, 5, 7);
    //std::cout << showResult(result, 5) << std::endl;
  }


  /**
   * The issue here is that initialization like:
   *
   *   Int x = x + 1;
   *
   * ... is allowed by C++ syntax. There is no way to prevent this, other
   * than hoping that the compiler flags this as warning. In the given example,
   * no warning is given.
   *
   * The only good way to deal with this, is to just be aware of it.
   *
   * This test only checks for `Int x = x;`, the simplest case possible.
   * Anything more elaborate, forget it. I've racked my brain on this, there is no salvation.
   */
  SUBCASE("Check init self issue") {
    ::log_to_cout(false);
    Log::log_to_cout(false);

    {
      auto k = compile(init_self_1_kernel);
      REQUIRE(k.has_errors());
    }

    {
      auto k = compile(init_self_2_kernel);
      REQUIRE(k.has_errors());
    }

    {
      auto k = compile(init_self_3_kernel);
      REQUIRE(k.has_errors());
    }

    ::log_to_cout(true);
    Log::log_to_cout(true);
  }

  Platform::use_main_memory(false);
}


//=============================================================================
// Test Block Syntax
//
// Notably, missing End's for If/Where/etc.
//=============================================================================

namespace {

void if_noend_kernel(Int::Ptr result) {
  Int i = 1;
  Int cond = 1;

  If (cond == 1)
    i = 42;
  }  // End

  *result = i;
}


void while_noend_kernel(Int::Ptr result) {
  Int i = 1;
  Int cond = 1;

  While (cond == 1)
    i = 42;
  }  // End

  *result = i;
}


void where_noend_kernel(Int::Ptr result) {
  Int i = 1;
  Int cond = 1;

  Where (cond == 1)
    i = 42;
  }  // End

  *result = i;
}


void for_noend_kernel(Int::Ptr result) {
  Int i = 1;

  For (Int cond = 0, cond < 1, cond++)
    i = 42;
  }  // End

  *result = i;
}


/**
 * Actually not part of `Test Block Syntax`.
 */
void int_div_kernel(Int::Ptr quotient, Int::Ptr remainder, Int a, Int b) {
  *quotient  = a / b;
  *remainder = a % b;
}


void nested_where_kernel(Int::Ptr ret) {
  Int tmp = 0;                comment("Before Where 1");

  // Doing `Where (index() % 2 == 0)` both both blocks
  // will use long integer division internally.
  // This is extremely heavy (resulting in a lot of generated code) and leads to wrong result;
  // it appears that first condition will then also be used for second block.
  Where ((index() & 1) == 0)
    tmp = 1;                  comment("Start Where 1, before Where 2");

    Where ((index() & 0x3) == 0)
      tmp = 2;                comment("Start Where 2");

      Where ((index() & 0xf) == 0)
        tmp = 3;                comment("Start Where 2");
      End
    End
  End

  *ret = tmp;
}  

} // anon namespace


TEST_CASE("Test issues [dsl][block]") {
  int const N = 1;

  Platform::use_main_memory(true);
  ::log_to_cout(false);
  Log::log_to_cout(false);

  Int::Array result(16*N);
  result.fill(-1);

  {
    auto k = compile(if_noend_kernel);
    k.load(&result).interpret();
    REQUIRE(k.has_errors());
  }

  {
    auto k = compile(while_noend_kernel);
    k.load(&result).interpret();
    REQUIRE(k.has_errors());
  }

  {
    auto k = compile(where_noend_kernel);
    k.load(&result).interpret();
    REQUIRE(k.has_errors());
  }

  {
    auto k = compile(for_noend_kernel);
    k.load(&result).interpret();
    REQUIRE(k.has_errors());
  }

  ::log_to_cout(true);
  Log::log_to_cout(true);
  Platform::use_main_memory(false);
}


TEST_CASE("Test integer division and remainder [dsl][intdiv]") {
  int const MAX_INT = 2147483647;  // inicates infinity

  Int::Array quotient(16);
  Int::Array remainder(16);

  auto k = compile(int_div_kernel);

  auto test = [&k, &quotient, &remainder] (int a, int b, int quotient_expected, int remainder_expected) {
    k.load(&quotient, &remainder, a, b).run();
    INFO(quotient.dump());
    INFO(remainder.dump());
    REQUIRE(quotient[0]  == quotient_expected);
    REQUIRE(remainder[0] == remainder_expected);
  };

  test( 22,   7,   3, 1);
  test(-22,   7,  -3, 1);
  test( 22,  -7,  -3, 1);
  test( 22,   5,   4, 2);
  test(128,   1, 128, 0);
  test(  1, 128,   0, 1);
  test( 33,  33,   1, 0);
  test(  0,   1,   0, 0);
  test( 32,   0,   MAX_INT, 0);
}


/**
 * Conclusion: Where-blocks can be nested, with some thought.
 *
 * The conditions of the encompassing Where-block also apply to the inner blocks.
 * This is as you want it.
 */
TEST_CASE("Test nested Where blocks [dsl][where]") {
  Int::Array result(16);
  std::vector<int> expected = {3, 0, 1, 0, 2, 0, 1, 0, 2, 0, 1, 0, 2, 0, 1, 0};

  auto k = compile(nested_where_kernel);
  k.load(&result).run();
  check_vector(result, 0, expected);
}


//=============================================================================
// DSL examinations
//
// Answer some burning questions on operations
//=============================================================================

namespace {

void tmu_kernel(Int::Ptr result) {
  result -= index();

  Int val = numQPUs()*index();

  *result = val;
  result.inc();

  // Write *some* values to same address
  result += index()/4;

  *result = val;
}

} // anon namespace


/**
 * Which vector element gets written on TMU write,
 * When multiple elements get written to same address?
 *
 * Answer:
 *   1. For single QPU, the highest index element written.
 *   2. For multi QPU's, the highest QPU index writing.
 *
 * The latter might only be applicable to QPU's running in perfect unison.
 * It might be that the longest running QPU gets to write last.
 */
TEST_CASE("Test edge cases of TMU [dsl][tmu]") {
  if (Platform::compiling_for_vc4()) {
    warn << "Skipping TMU write for vc4";
  } else {
    Int::Array result(2*16);
    result.fill(0);

    std::vector<int> expected = {
      15, 0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
       3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    auto k = compile(tmu_kernel);

    //
    // Test single QPU
    //
    k.load(&result).run();
    //warn << result.dump();
    check(result, expected, "TMU 1 QPU  write same address");

    //
    // Test multiple QPU's
    //
    const int NumQPUs = 7;  // All values appear to work fine

    for (int i = 0; i < (int) expected.size(); ++i) {
      expected[i] *= NumQPUs;
    }

    k.setNumQPUs(NumQPUs);
    k.load(&result).run();
    //warn << result.dump();
    check(result, expected, "TMU multi QPU's  write same address");
  }
}
