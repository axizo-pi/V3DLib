#include <V3DLib.h>
#include "support/support.h"
#include "Support/Helpers.h"
#include "global/log.h"

using namespace Log;

namespace {

using namespace V3DLib;

int COUNT      = 48;
int DummyCount = 20;

struct Context {
	Context() {
  	Count = COUNT;
	}

  Int Count;
};


void dummy_calculation(Float *dummy) {
  for (int k = 0; k < DummyCount; k++) {
    dummy[k] *= 1.5f;

    if (k > 0) {
      dummy[k] += dummy[k - 1] - 2.5;     comment("add dummy values");
      dummy[k] *= dummy[k];
    }
  }
}


/**
 * @brief Examine loop issues
 *
 * This appears to be fixed, issues with DMA/TMU read write
 *
 * See unit tests in testMutex for while and for.
 *
 * ---------------------------------------------------------------
 *
 * **Hypothesis:**  
 * Loops with loops influence outer loop conditions.
 * _Conclusion_: Nope.
 *
 * **Hypothesis:**  
 * Usage of Float pointers within loop  
 * _Conclusion_: Nope.
 *
 * **Hypothesis:**  
 * Variable initialization within loops  
 * _Conclusion_: Nope.
 *
 * **Hypothesis:**  
 * Function calls within kernel affect loop  
 * _Conclusion_: Nope, works fine.
 *
 * **Hypothesis:**  
 * `barrier()` at end of loop affects the loop.  
 * _Conclusion_: Nope, works fine.
 *
 * **Hypothesis:**  
 * The loop jump is too big.
 * Following adds dummy operations to the loop, to enlarge it.  
 * In Gravity, the backwards jump is > -4000
 * 
 * _Conclusion_: This is not the issue, got to following:
 *
 *      branch.any_zc -24720  # Jump to label L0
 *
 *
 * Notes
 * =====
 *
 *  - int DummyCount = 1024;   - didn't complete, probably hang
 *
 *  - vc4: When more registers are required than present, the following exception occurs:
 *
 *     vc4 emit_opcodes() discrepancy in opcode and target code size.opcode size: 1,
 *     target code size: 325
 */
void loop_kernel(Int::Ptr dst, Float::Ptr in_dummy_ptr) {
  Context c;
  comment("Start loop kernel");

  Int count = 0;

  Float::Ptr dummy_ptr = in_dummy_ptr;
  Float dummy[DummyCount];

  for (int k = 0; k < DummyCount; k++) {
    dummy[k]  = 0.125f;                   comment("Init dummy");
  }

  For (Int i = 0, i < c.Count, i++)
    For (Int j = 0, j < 16, j++)
      dummy_calculation(dummy);
    End

    count++;

    barrier();

    *dummy_ptr = dummy[0];
    *dst = count;                             comment("store count");

    For (Int k = 0, k < 16, k++)
      Int dummy2 = 3*16 -1;
    End
  End

}

}  // anon namespace


//
// Following is an issue on vc4 Gravity.
// This is a testbed for sorting out what is happening.
//
TEST_CASE("Test loop counter [loop]") {
  if (Platform::compiling_for_vc4()) {
    // The used version of barrier() is only for v3d
    return;
  }

  SUBCASE("Test loop with context") {
    Float::Array dummy(16);
    Int::Array result(16);
    result.fill(-1);

    auto k = compile(loop_kernel);
		//to_file("loop_kernel.txt", k.dump());
    k.load(&result, &dummy).run();

    for (int i = 0; i < (int) result.size(); ++i) {
      INFO("i: " << i);
      REQUIRE(result[i] == COUNT);
    }
  }
}
