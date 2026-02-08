//
// Kernels for use with the 'Rot3D' example.
//
// These are kept in a separate file so that they can also be used
// in the unit tests.
//
// After judicious profiling, kernel 1a has been selected as best vector kernel.
// This will now be the standard, all other vector kernels die.
//
// ============================================================================
#include "Rot3D.h"
#include "Source/Functions.h"
#include <math.h>             // sinf(), cosf()

namespace kernels { 

using namespace V3DLib;


// ============================================================================
// Scalar version
// ============================================================================

/**
 * Scalar kernel to rotate around x- and/or y- and/or z-axis.
 *
 * All angles are performed for in the calculation.
 * The order of rotation is:
 *
 * - Around x-axis
 * - Around y-axis
 * - Around z-axis
 *
 * @param n     Number of elements in passed float arrays
 * @param rot_x Angle to which rotate clockwise around the x-axis.
 *              The value is a multiple of `PI`.
 * @param rot_y Angle to which rotate clockwise around the y-axis.
 *              The value is a multiple of `PI`.
 * @param rot_z Angle to which rotate clockwise around the z-axis.
 *              The value is a multiple of `PI`.
 * @param x     Array containing x-component of vectors to rotate
 * @param x     Array containing y-component of vectors to rotate
 * @param z     Array containing z-component of vectors to rotate
 */
void scalar_rot3D(int n, float rot_x, float rot_y, float rot_z, float *x, float *y, float *z) {
  const float PI = (float) 3.14159;

  rot_x *= PI;
  rot_y *= PI;
  rot_z *= PI;

  auto cos_z = cos(rot_z);
  auto sin_z = sin(rot_z);

  for (int i = 0; i < n; i++) {
    float x_prev;
    float y_prev;
    
    float z_prev;

    // Rotation around x-axis
    y_prev = y[i];
    z_prev = z[i];

    y[i] = y_prev * cos(rot_x) - z_prev * sin(rot_x);
    z[i] = y_prev * sin(rot_x) + z_prev * cos(rot_x);

    // Rotation around y-axis
    x_prev = x[i];
    z_prev = z[i];

    x[i] = x_prev *  cos(rot_y) + z_prev * sin(rot_y);
    z[i] = x_prev * -sin(rot_y) + z_prev * cos(rot_y);

    // Rotation around z-axis
    x_prev = x[i];
    y_prev = y[i];

    x[i] = x_prev * cos_z - y_prev * sin_z;
    y[i] = x_prev * sin_z + y_prev * cos_z;
  }
}


namespace {

// ============================================================================
// Kernel Helpers
// ============================================================================

/**
 * Rotation around x-axis.
 */
void rotate_x(Float &y_prev, Float &z_prev, Float &cos_x, Float &sin_x, Float::Ptr y, Float::Ptr z) {
  Float y_new = y_prev * cos_x - z_prev * sin_x;
  Float z_new = y_prev * sin_x + z_prev * cos_x;

  *y = y_new;
  *z = z_new;

  y_prev = y_new;
  z_prev = z_new;
}


/**
 * Rotation around y-axis.
 */
void rotate_y(Float &x_prev, Float &z_prev, Float &cos_y, Float &sin_y, Float::Ptr x, Float::Ptr z) {
  Float x_new = x_prev *    cos_y + z_prev * sin_y;
  Float z_new = x_prev * -1*sin_y + z_prev * cos_y;

  *x = x_new;
  *z = z_new;

  x_prev = x_new;
  z_prev = z_new;
}


/**
 * Rotation around z-axis.
 */
void rotate_z(Float &x_prev, Float &y_prev, Float &cos_z, Float &sin_z, Float::Ptr &x, Float::Ptr &y) {
  Float x_new = x_prev * cos_z - y_prev * sin_z;
  Float y_new = x_prev * sin_z + y_prev * cos_z;

  *x = x_new;
  *y = y_new;

  x_prev = x_new;
  y_prev = y_new;
}

}  // anon namespace



// ============================================================================
// Vector Kernel
// ============================================================================

/**
 *
 * Incoming angle values must be multiples of 2*PI.
 */
void vector_rot3D(Int n, Float rot_x, Float rot_y, Float rot_z, Float::Ptr x, Float::Ptr y, Float::Ptr z) {

  Int step         = numQPUs() << 4;
  Int start_offset = me()*16;

  x += start_offset;
  y += start_offset;
  z += start_offset;

  Float cos_x = cos(rot_x);
  Float sin_x = sin(rot_x);
  Float cos_y = cos(rot_y);
  Float sin_y = sin(rot_y);
  Float cos_z = cos(rot_z);
  Float sin_z = sin(rot_z);

  Float x_prev;
  Float y_prev;
  Float z_prev;

  //
  // Still errors in output.
  //
  // Loop taken from Gravity kernel_step().
  // Expected to be better but isn't
  // Try insisting that n is multiple of 16
  //For (Int i = me(), i < (n >> 4), i += numQPUs())
  //
  For (Int i = 0, i < n, i += step)
    x_prev = *x;
    y_prev = *y;
    z_prev = *z;

    rotate_x(y_prev, z_prev, cos_x, sin_x, y, z);
    rotate_y(x_prev, z_prev, cos_y, sin_y, x, z);
    rotate_z(x_prev, y_prev, cos_z, sin_z, x, y);

    x += step;
    y += step;
    z += step;
  End
}

}  // namespace kernels
