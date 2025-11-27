/**
 * Support for Revese Neural Networks
 */
#include "Support/Settings.h"
#include "Source/Float.h"
#include "Support/basics.h"
#include "Kernel.h"
#include "Source/Functions.h"  // rotate

using namespace V3DLib;
using namespace Log;

const int N = 16;

Settings settings;

/**
 * Source: https://stackoverflow.com/questions/686353/random-float-number-generation
 */
float frand() {
  static bool did_init = false;
  if (!did_init) {
    srand (static_cast <unsigned> (time(0)));
    did_init = true;
  }

  // Generate a number from 0.0 to 1.0, inclusive.
  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

/*
  // Generate a number from 0.0 to some arbitrary float, X
  const float X = 1.0f;
  float r2 = static_cast <float> (rand()) / (static_cast <float> (((float) RAND_MAX)/X));

  // Generate a number from some arbitrary LO to some arbitrary HI:
  const float LO = 0.0f;
  const float HI = 1.0f;
  float r3 = LO + static_cast <float> (rand()) /( static_cast <float> (((float) RAND_MAX)/(HI-LO)));
*/

  return r;
}


void frand_array(Float::Array &rhs) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    rhs[i] = frand();
  }
}



void run_scalar(Float::Array const &a, Float::Array const &b, float &res) {
  assert(a.size() == b.size());

  res = 0;

  for (int i = 0; i < (int) a.size(); ++i) {
    res += a[i]*b[i];
  }
}


template<int const N>
void kernel(Float::Ptr ptr_a, Float::Ptr ptr_b, Float::Ptr result) {
  assert(N > 0);
  Float tmp = 0;

  tmp += (*ptr_a)*(*ptr_b);
  for (int i = 1; i < N; ++i) {
    ptr_a.inc();
    ptr_b.inc();
    tmp += (*ptr_a)*(*ptr_b);
  }

  Float res = 0;
  rotate_sum(tmp, tmp);
  res.set_at(0, tmp);

  *result = res;
}


int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

  Float::Array a(16*N);
  Float::Array b(16*N);
  frand_array(a);
  frand_array(b);

  float scalar_res = 0;
  run_scalar(a, b, scalar_res);
  warn << "run_scalar result: " << scalar_res;

  Float::Array result(16);

  auto k = compile(kernel<N>, settings);
  k.load(&a, &b, &result).run();

  warn << "kernel result: " << result[0];
  std::string buf;
  for (int i = 0; i < (int) result.size(); ++i) {
    buf << result[i] << ", ";
  }
  warn << "kernel result: " << buf;

  return 0;
}
