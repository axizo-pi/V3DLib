#include "scalar.h"
#include "settings.h"
#include "Support/debug.h"
#include "Support/Timer.h"
#include "Kernels/Rot3D.h"
#include <iostream>

using namespace std;
using namespace V3DLib;
using namespace kernels;

namespace {
	
// ============================================================================
// Scalar Support
// ============================================================================

void init_arrays(float *x, float *y, float *z, int size) {
	assert(size % 16 == 0);
	assert(size >= 16);

	auto init_v = [x, y, z] (int index, float in_x, float in_y, float in_z) {
		x[index] = in_x;
		y[index] = in_y;
		z[index] = in_z;
	};

	// First, initialize basic vectors
	init_v(0,  1,  0,  0);
	init_v(1,  0,  1,  0);
	init_v(2,  0,  0,  1);
	init_v(3, -1,  0,  0);
	init_v(4,  0, -1,  0);
	init_v(5,  0,  0, -1);
	init_v(6,  1,  1,  0);
	init_v(7,  1,  0,  1);
	init_v(8,  0,  1,  1);

	// Add some ostensibly random stuff
  for (int i = 9; i < size; i++) {
    x[i] = (float) ((i +2)% 4);
    y[i] = (float) (i % 4);
    z[i] = 1;

    x[i] *= (float) (i + 1);
    y[i] *= (float) (i + 1);
    z[i] *= (float) (i + 1);
  }
}


void disp_arrays(float *x, float *y, float *z, int size) {
  if (!settings.show_results) return;

  for (int i = 0; i < settings.num_vertices; i++) {
		cout << x[i] << ", " << y[i] << ", " << z[i] << "\n";
  }
}

} // anon namespace


void run_scalar_kernel() {
  const int size = settings.num_vertices;

  // Allocate and initialise
  float* x = new float[size];
  float* y = new float[size];
  float* z = new float[size];
  init_arrays(x, y, z, size);
  disp_arrays(x, y, z, size);

  if (!settings.compile_only) {
    Timer timer;  // Time the run only
    rot3D(size, settings.rot_x, settings.rot_y, settings.rot_z, x, y, z);
    timer.end(!settings.silent);
  }

  disp_arrays(x, y, z, size);

  delete [] z;
  delete [] y;
  delete [] x;
}

