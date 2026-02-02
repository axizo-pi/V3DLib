#include "doctest.h"
#include <math.h>
#include "Kernels/Rot3D.h"
#include "LibSettings.h"

using namespace kernels;


// ============================================================================
// Support routines
// ============================================================================

/**
 * Convenience method to initialize arrays.
 */
template<typename Arr>
void initArrays(Arr &x, Arr &y, int size, float mult = 1.0f) {
  for (int i = 0; i < size; i++) {
    x[i] = mult*((float) i);
    y[i] = mult*((float) i);
  }
}


template<typename Array1, typename Array2>
void compareResults(
  Array1 &x1,
  Array1 &y1,
  Array2 &x2,
  Array2 &y2,
  int size,
  const char *label,
  bool compare_exact = true) {
  for (int i = 0; i < size; i++) {
    INFO("Comparing " << label << " for index " << i);
    if (compare_exact) {
      INFO("y2[" << i << "]: " << y2[i]);
      REQUIRE(x1[i] == x2[i]);
      REQUIRE(y1[i] == y2[i]);
    } else {
      INFO("x1[" << i << "]: " << x1[i]);
      REQUIRE(x1[i] == doctest::Approx(x2[i]).epsilon(0.001));
      REQUIRE(y1[i] == doctest::Approx(y2[i]).epsilon(0.001));
    }
  }
}


// ============================================================================
// The actual tests
// ============================================================================

TEST_CASE("Test working of Rot3D example [rot3d]") {
  // Number of vertices and angle of rotation
  const int N = 16*12*10;  // 1920
  const float THETA = (float) 3.14159;

  /**
   * Check that the Rot3D kernels return precisely what we expect.
   *
   * vc4: The scalar version of the algorithm may return slightly different
   *      values than the actual QPU's, but they should be close. This is
   *      because the hardware QPU's round downward in floating point
   *      calculations.
   * vc6, vc7: No rounding errors, calc's should match exact.
   *
   * If the code is compiled for emulator only (QPU_MODE=0), this
   * test will fail.
   *
   * vc4: No problem at all when all kernels loaded in same BO.
   * v3d: rot3D_1 and rot3D_2 can be loaded together, adding the 3-kernels
   *      leads to persistent timeout:
   *
   *    ERROR: v3d_wait_bo() ioctl: Timer expired
   *
   * Running the test multiple times leads to the unit tests hanging (and eventually losing contact with the Pi4.
   *
   * For this reason, separate contexts are used for each kernel. This results in the BO being completely
   * cleared for the next kernel.
   *
   */
  SUBCASE("All kernel versions should return the same") {
    //
    // Run the scalar version
    //

    // Allocate and initialise
    float* x_scalar = new float [N];
    float* y_scalar = new float [N];
    float* z_scalar = new float [N];
    initArrays(x_scalar, y_scalar, N);

		// Run scalar
		scalar_rot3D(N, 0, 0, 1, x_scalar, y_scalar, z_scalar);

    // Storage for first kernel results
    float* x_1 = new float [N];
    float* y_1 = new float [N];

		//
    // Compare scalar with output of vector kernel with 1 QPU - may not be exact
		//
    {
      INFO("Running kernel 1 (always 1 QPU)");
      Float::Array x(N), y(N);
      initArrays(x, y, N);

      auto k = compile(vector_rot3D);
      k.load(N, cosf(THETA), sinf(THETA), &x, &y).run();
      compareResults(x_scalar, y_scalar, x, y, N, "Rot3D", false);

      // Save results for compare with other kernels 
      for (int i = 0; i < N; ++i) {
        x_1[i] = x[i];
        y_1[i] = y[i];
      }
    }

		//
    // Compare scalar with output of vector kernel with mulitple QPU's - also not exact
		//
    {
      INFO("Running kernel with 8 QPUs");
      Float::Array x(N), y(N);
      initArrays(x, y, N);

      auto k = compile(vector_rot3D);
      k.setNumQPUs(8);

      k.load(N, cosf(THETA), sinf(THETA), &x, &y).run();
			INFO("Loaded and ran the kernel");
      compareResults(x_scalar, y_scalar, x, y, N, "Rot3D_3 8 QPUs", false);
    }

    delete [] x_1;
    delete [] y_1;
    delete [] z_scalar;
    delete [] y_scalar;
    delete [] x_scalar;
  }
}
