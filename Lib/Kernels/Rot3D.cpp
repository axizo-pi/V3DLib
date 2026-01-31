//
// Kernels for use with the 'Rot3D' example.
//
// These are kept in a separate file so that they can also be used
// in the unit tests.
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

void rot3D(int n, float rot_x, float rot_y, float rot_z, float *x, float *y, float *z) {
  const float PI = (float) 3.14159;

	rot_x *= PI;
	rot_y *= PI;
	rot_z *= PI;

  for (int i = 0; i < n; i++) {
		// Rotation around x-axis
    float y_prev = y[i];
    float z_prev = z[i];

    y[i] = y_prev * cos(rot_x) - z_prev * sin(rot_x);
    z[i] = y_prev * sin(rot_x) + z_prev * cos(rot_x);

		// Rotation around y-axis
    float x_prev = x[i];
    z_prev = z[i];

    x[i] = x_prev *  cos(rot_y) - z_prev * sin(rot_y);
    z[i] = x_prev * -sin(rot_y) + z_prev * cos(rot_y);

		// Rotation around z-axis
    x_prev = x[i];
    y_prev = y[i];

    x[i] = x_prev * cos(rot_z) - y_prev * sin(rot_z);
    y[i] = x_prev * sin(rot_z) + y_prev * cos(rot_z);
  }
}


// ============================================================================
// Kernel version 1
// ============================================================================

void rot3D_1(Int n, Float cosTheta, Float sinTheta, Float::Ptr x, Float::Ptr y) {
  For (Int i = 0, i < n, i += 16)
    Float xOld = x[i];
    Float yOld = y[i];
    x[i] = xOld * cosTheta - yOld * sinTheta;
    y[i] = yOld * cosTheta + xOld * sinTheta;
  End
}


void rot3D_1a(Int n, Float cosTheta, Float sinTheta, Float::Ptr x, Float::Ptr y) {

  Int step = numQPUs() << 4;
  x += me()*16;
  y += me()*16;

  auto read = [] (Float &dst, Float::Ptr &src) {
    dst = *src;
    //gather(src);
    //receive(dst);
  };

  For (Int i = 0, i < n, i += step)
    Float xOld;
    Float yOld;

    read(xOld, x);
    read(yOld, y);

    *x = xOld * cosTheta - yOld * sinTheta;
    *y = yOld * cosTheta + xOld * sinTheta;

    x += step;
    y += step;
  End
}


// ============================================================================
// Kernel version 2
// ============================================================================

void rot3D_2(Int n, Float cosTheta, Float sinTheta, Float::Ptr x, Float::Ptr y) {
  Int inc = numQPUs() << 4;
  Float::Ptr p = x + me()*16;
  Float::Ptr q = y + me()*16;

  gather(p); gather(q);
 
  Float xOld, yOld;
  For (Int i = 0, i < n, i += inc)
    gather(p+inc); gather(q+inc); 
    receive(xOld); receive(yOld);

    *p = xOld * cosTheta - yOld * sinTheta;
    *q = yOld * cosTheta + xOld * sinTheta;
    p += inc; q += inc;
  End

  receive(xOld); receive(yOld);
}


// ============================================================================
// Kernel version 3
// ============================================================================

namespace {
  int N = -1;       // Number of elements in incoming arrays for rot3D_3
  int numQPUs = -1; // Number of QPUs to use for rot3D_3
}  // anon namespace


void rot3D_3(Float cosTheta, Float sinTheta, Float::Ptr x, Float::Ptr y) {
  assert(N != -1);
  assert(numQPUs != -1);
  assertq(N % (16*numQPUs) == 0, "N must be a multiple of '16*numQPUs'");

  int size = N/numQPUs;
  Int count = size >> 4;

  Int adjust = me()*size;

  Float::Ptr p_src = x + adjust;
  Float::Ptr q_src = y + adjust;
  Float::Ptr p_dst = x + adjust;
  Float::Ptr q_dst = y + adjust;

  gather(p_src, q_src);
 
  Float xOld, yOld;
  For (Int i = 0, i < count, i++)
    receive(xOld, p_src);
    receive(yOld, q_src);

    *p_dst = xOld * cosTheta - yOld * sinTheta;
    *q_dst = yOld * cosTheta + xOld * sinTheta;

    p_dst.inc();
    q_dst.inc();
  End

  receive();
}


/**
 * Using decorator to avoid `N/numQPUs()` in source language code.
 * TODO retest to see effect of that division, optimize it if a problem. 
 */
FuncType *rot3D_3_decorator(int dimension, int in_numQPUs) {
  assert(dimension > 0);
  assertq(dimension % 16 == 0, "dimension must be a multiple of 16");
  // TODO perhaps assert in_numQPUs as well

  N = dimension;
  numQPUs = in_numQPUs;
  return rot3D_3;
}

}  // namespace kernels
