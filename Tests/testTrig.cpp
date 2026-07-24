#include <V3DLib.h>
#include "support/support.h"
#include "Support/pgm.h"
#include <cmath>              // M_PI

using namespace V3DLib;
using namespace std;

//=============================================================================
// Test Trigonometric Functions
//=============================================================================

namespace {

//=============================================================================
// Helper methods
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


/**
 * Calculate max abs difference for arrays
 */
float max_abs_value(std::vector<float> const &a, float const *b) {
  REQUIRE(b != nullptr);

  float ret = -1.0f;

  for (int i = 0; i < (int) a.size(); ++i) {
    float diff = abs(a[i] - b[i]);
    if (ret == -1.0f || ret < diff) {
      ret = diff;
    }
  }

  return ret;
} 


/**
 * Generate cos values to compare with
 */
std::vector<float> lib_cos_values(int size, float freq = -1.0f, float offset = 0.0f) {
  if (freq == -1.0f) {
    freq = 1.0f/((float) size);
  }

  std::vector<float> ret;
  ret.resize(size);

  for (int x = 0; x < size; ++x) {
    ret[x] = cos((float) (freq*(2*M_PI)*(((float) x) - offset)));
  }

  return ret;
}


std::vector<float> lib_sin_values(int size, float freq = -1.0f, float offset = 0.0f) {
  if (freq == -1.0f) {
    freq = 1.0f/((float) size);
  }

  std::vector<float> ret;
  ret.resize(size);

  for (int x = 0; x < size; ++x) {
    ret[x] = sin((float) (freq*(2*M_PI)*(((float) x) - offset)));
  }

  return ret;
}


std::vector<float> lib_neg_sin_values(int size, float freq = -1.0f, float offset = 0.0f) {
  if (freq == -1.0f) {
    freq = 1.0f/((float) size);
  }

  std::vector<float> ret;
  ret.resize(size);

  for (int x = 0; x < size; ++x) {
    ret[x] = sin( -((float) (freq*(2*M_PI)*(((float) x) - offset))));
  }

  return ret;
}


std::vector<float> lib_tanh_values(int size, float min_x = -2.0f, float max_x = 2.0f) {
  assert(max_x > min_x);

  std::vector<float> ret;
  ret.resize(size);

  float step = (max_x - min_x)/((float) size);

  for (int n = 0; n < size; ++n) {
    float x = min_x + ((float) n)*step; //((float) (freq*(((float) n) - offset)));
    ret[n] = tanh(x);
  }

  return ret;
}


//=============================================================================
// Kernels
//=============================================================================

void cosine_kernel(Float::Ptr result, Int numValues, Float freq, Int offset) {
  For (Int n = 0, n < numValues, n += 16)
    Float x = freq*toFloat(n + index() - offset);
    *result = functions::cos(x);
    result.inc();
  End
}


/*
  Currently not used

void sine_kernel(Float::Ptr result, Int numValues, Float freq, Int offset) {
  For (Int n = 0, n < numValues, n += 16)
    Float x = freq*toFloat(n + index() - offset);
    *result = functions::sin(x);
    result.inc();
  End
}
*/


void sincos_kernel(Float::Ptr result, Int size) {
  Int count = size >> 4;

  For (Int n = 0, n < count, n++)
    Float param = toFloat((n << 4) + index())/toFloat(size);

    Float val  = functions::sin(param);
    *result = val;  result.inc();
  End

  For (Int n = 0, n < count, n++)
    Float param = toFloat((n << 4) + index())/toFloat(size);

    Float val  = functions::sin(param);
    *result = val;  result.inc();
  End

  For (Int n = 0, n < count, n++)
    Float param = toFloat((n << 4) + index())/toFloat(size);

    Float instr_val = sin(param);
    *result = instr_val;  result.inc();
  End

  For (Int n = 0, n < count, n++)
    Float param = toFloat((n << 4) + index())/toFloat(size);

    Float instr_val = sin(-1*param);
    *result = instr_val;  result.inc();
  End

  For (Int n = 0, n < count, n++)
    Float param = toFloat((n << 4) + index())/toFloat(size);

    Float instr_val = cos(param);
    *result = instr_val;  result.inc();
  End
}


void tanh_kernel(Float::Ptr result, Int size, Float min_x, Float max_x) {
  Int count = size >> 4;
  Float step = (max_x - min_x)/toFloat(size);

  For (Int n = 0, n < count, n++)
    Float param = min_x + toFloat((n << 4) + index())*step;

    Float val  = tanh(param);
    *result = val;  result.inc();
  End
}
  
}  // namespace


//=============================================================================
// Unit tests
//=============================================================================

TEST_CASE("Test functions [trig][func]") {

  /**
   * NOTE: Remember, sin/cos normalized on 2*M_PI
   */
  SUBCASE("Test trigonometric functions") {
    float const MAX_DIFF = 0.57f;  // Test value for extra_precision == false

    const int size   = 1000;
    const int offset = size/2;
    const float freq = (float) (1.0f/((double) size));

    auto lib_cos = lib_cos_values(size, freq, offset);  // cos lib values, to compare with

    //
    // Calc with scalar kernel
    //
    float scalar_cos[size];

    {
      for (int x = 0; x < size; ++x) {
        scalar_cos[x] = functions::scalar::cos(freq*((float) (x - offset)));
      };

      float max_diff = calc_max_diff(scalar_cos, lib_cos, size); 
      INFO("Max diff: " << max_diff);
      REQUIRE(max_diff < MAX_DIFF);
    }

    //
    // Calc with QPU kernel
    //
    Float::Array qpu_cos(size);
    Float::Array qpu_sin(size);

    {
      auto k = compile(cosine_kernel);
      k.load(&qpu_cos, size, freq, offset).run();

      float max_diff = calc_max_diff(lib_cos, qpu_cos, size); 
      INFO("Max diff: " << max_diff);
      REQUIRE(max_diff < MAX_DIFF);
    }

    PGM pgm(size, 400);
    pgm.plot(lib_cos, 64)
       .plot(qpu_cos.ptr(), size, 32)
       .plot(qpu_sin.ptr(), size, 32)
       .save((test_path() + "/cos_plot.pgm").c_str());
  }

}


TEST_CASE("Test sin/cos instructions [dsl][sincos]") {
  int const N = 5*16;

  Float::Array result(5*N);
  auto lib_sin     = lib_sin_values(N);  // lib values, to compare with
  auto lib_cos     = lib_cos_values(N);  // lib values, to compare with
  auto lib_neg_sin = lib_neg_sin_values(N);

  auto k = compile(sincos_kernel);
  //to_file("sincos_kernel.txt", k.dump());
  k.load(&result, N).run();

  float const hi_precision = 1.2e-3f;
  float const lo_precision = 5.7e-2f;

  // vc4 will use the lo-res sin function,
  // v3d the will use hardware, which is precise
  float const qpu_precision = (Platform::run_vc4())?lo_precision:1.0e-6f;

  {
    float diff = max_abs_value(lib_sin, result.ptr());
    INFO("max abs diff hi-prec sin: " << diff);
    REQUIRE(diff <= hi_precision);
  }

  {
    float diff = max_abs_value(lib_sin, result.ptr() + N);
    INFO("max abs diff lo-prec sin: " << diff);
    REQUIRE(diff <= lo_precision);
  }

  {
    float diff = max_abs_value(lib_sin, result.ptr() + 2*N);
    INFO("max abs diff v3d sin: " << diff);
    REQUIRE(diff <= qpu_precision);
  }

  // There were issues here, check all vc's: working pi5
  //
  //Log::warn << showResult(lib_neg_sin, 0, N);
  //Log::warn << showResult(result, 3, N);
  {
    float diff = max_abs_value(lib_neg_sin, result.ptr() + 3*N);
    INFO("max abs diff v3d sin: " << diff);
    REQUIRE(diff <= qpu_precision);
  }

  {
    float diff = max_abs_value(lib_cos, result.ptr() + 4*N);
    INFO("max abs diff v3d sin: " << diff);
    REQUIRE(diff <= qpu_precision);
  }
}


TEST_CASE("Test tanh [dsl][tanh]") {
  int const N = 48;          // 128;
  float const min_x = -2.0f; //-6.0f;
  float const max_x =  2.0f; // 6.0f;

  Float::Array result(N);
  result.fill(2.0f);
  auto lib_tanh = lib_tanh_values(N, min_x, max_x);

  auto k = compile(tanh_kernel);
  //to_file("tanh_kernel.txt", k.dump());
  k.load(&result, N, min_x, max_x).run();

  //Log::warn << "\n  " << showExpected(lib_tanh)
  //          << "\n  " << showResult(result, 0, N);
  //Log::warn << "Max diff: " << calc_max_diff(lib_tanh, result, N);

  const float PRECISION = Platform::compiling_for_vc4()?1.0e-4f:5.0e-7f;

  float max_diff = calc_max_diff(lib_tanh, result, N);
  REQUIRE(max_diff < PRECISION);
}
