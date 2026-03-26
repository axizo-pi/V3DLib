#include <V3DLib.h>
#include "support/support.h"
#include "Support/Helpers.h"
#include "LibSettings.h"

namespace {

using namespace V3DLib;

template<typename T, typename Ptr>
void prefetch_kernel(Ptr dst, Ptr src) {

  //
  // The usual way of doing things
  //

  T input = *src;  comment("Start regular fetch/store");

  src.inc();
  *dst = input;
  dst.inc();

  T inputa = *src;
  src.inc();
  *dst = inputa;
  dst.inc();

  //
  // With regular gather
  //
  input = -2.0f; comment("Start regular gather");

  gather(src);
  gather(src + 16);
  receive(input);
  *dst = input;
  dst.inc();
  receive(input);
  *dst = input;
  dst.inc();


  //
  // Now with prefetch
  //
  *dst = 123;                 comment("Start prefetch");

  src += 32;
  input = -3;
  T input2 = -4;
  T a = 1357;                 // Interference
  prefetch(input, src);       comment("First prefetch");
  T b = 2468;                 // Interference
  prefetch(input2, src);      comment("Second prefetch");
  T input3 = -6;
  prefetch(input3, src + 0 );  comment("Test usage PointerExpr");

  *dst = input;
  dst.inc();
  *dst = input2;
  dst.inc();
  *dst = input3;
}


template<int const N>
void multi_prefetch_kernel(Int::Ptr result, Int::Ptr src) {
  Int a = 234;  // Best to have the receiving var out of the loop,
                // otherwise it might be recreated in a different register of the rf
                // (Unproven but probably correct hypothesis)

  for (int i = 0; i < N; ++i) {
    prefetch(a, src);
    *result = 2*a;
    result += 16;
  }
}

}  // anon namespace


TEST_CASE("Test prefetch on stmt stack [prefetch]") {
  LibSettings::tmu_load tmu(false);  // TMU load fails, DMA load works fine

  int const N = 7;

  SUBCASE("Test prefetch with integers") {
    Int::Array src(16*N);
    for (int i = 0; i < (int) src.size(); ++i) {
      src[i] = i + 1;
    }

    Int::Array result(16*N);
    result.fill(-1);

    auto k = compile(prefetch_kernel<Int, Int::Ptr>);
		to_file("prefetch_kernel<Int>.txt", k.dump());
    k.load(&result, &src).run();
  
    for (int i = 0; i < (int) result.size(); ++i) {
      INFO("i: " << i);
      REQUIRE(result[i] == src[i]);
    }
  }


  SUBCASE("Test prefetch with floats") {
    Float::Array src(16*N);
    for (int i = 0; i < (int) src.size(); ++i) {
      src[i] = (float) (i + 1);
    }

    Float::Array result(16*N);
    result.fill(-1);

    auto k = compile(prefetch_kernel<Float, Float::Ptr>);
    k.load(&result, &src).run();

    for (int i = 0; i < (int) result.size(); ++i) {
      INFO("i: " << i);
      REQUIRE(result[i] == src[i]);
    }
  } 


  SUBCASE("Test more fetches than prefetch slots") {
    const int N = 10;  // anything over 8 will result in prefetches after loads

    Int::Array src(16*N);
    for (int i = 0; i < (int) src.size(); ++i) {
      src[i] = i + 1;
    }

    Int::Array result(16*N);
    result.fill(-1);

    auto k = compile(multi_prefetch_kernel<N>);
    k.load(&result, &src).run();

    for (int i = 0; i < (int) result.size(); ++i) {
      INFO("i: " << i);
      REQUIRE(result[i] == 2*src[i]);
    }
  }
}
