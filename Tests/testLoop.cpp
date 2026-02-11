#include <V3DLib.h>
#include "support/support.h"
#include "Support/Helpers.h"

namespace {

using namespace V3DLib;

int COUNT = 128;

struct Context {
	Context() {
  	Count = COUNT;
	}

  Int Count;
};


void loop_kernel(Int::Ptr dst) {
  Context c;

  Int count = 0;
  Float dummy = 0.125f;

  For (Int i = 0, i < c.Count, i++)
    dummy *= 1.5;
    count++;
  End

  *dst = count;
}

}  // anon namespace


//
// Following is an issue on vc4 Gravity.
// Following doesn't reproduce it.
//
TEST_CASE("Test loop counter [loop]") {

  SUBCASE("Test loop with context") {
    Int::Array result(16);
    result.fill(-1);

    auto k = compile(loop_kernel);
		to_file("loop_kernel.txt", k.dump());
    k.load(&result).run();
  
    for (int i = 0; i < (int) result.size(); ++i) {
      INFO("i: " << i);
      REQUIRE(result[i] == COUNT);
    }
  }
}
