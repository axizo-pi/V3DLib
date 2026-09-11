#include "doctest.h"
#include "V3DLib.h"
#include "Support/Helpers.h"         // resize_16()
#include "support/check.h"
#include "Source/GlobalConstants.h"  // MaxFloat()
#include <vector>

using namespace V3DLib;

namespace {

void valid_kernel(
  Int::Ptr ret_valid,
  Int      N_spheres // Blocks of 16
) {
  Float ray_t_min = 0.001f;
  //Float ray_t_max = MaxFloat();
  Float ray_t_max = 4;

  For (Int i = 0, i < N_spheres, i++)
    Int valid = 1;                                                 comment("init valid");

    Float h      = toFloat(index() - 8);
    Float a      = 1.0f;
    Float sqrtd  = 2.0f;
    Float root   = 0.0f;
    Float root_2 = 0.0f; sub_header("Start test root");

    Where (valid == 1)
      root = (h - sqrtd) / a;
      root_2 = (h + sqrtd) / a;

      Where (!(ray_t_min < root && root < ray_t_max))
        root = root_2;

        //valid = 0;

        Where (!(ray_t_min < root && root < ray_t_max))
          valid = 0;
        End
      End
    End

    *ret_valid = valid;
    ret_valid.inc();
  End
}

} // anon namespace


/**
 * This tests isolates an issue in RayTracing.
 *
 * valid fiels sometimes incorrect, even if the
 * discriminant values are exact.
 */
TEST_CASE("Test valid [issues][raytracing]") {
  Int::Array ret_valid(16);

  auto k = compile(valid_kernel);
  k.load(&ret_valid, 1).run();

  warn << "test_valid: " << showResult(ret_valid, 0, 16);
}
