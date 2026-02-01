#include "scalar.h"
#include "settings.h"
#include "Support/debug.h"
#include "Support/Timer.h"
#include "Kernels/Rot3D.h"
#include <iostream>
#include "stlfile.h"

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

  for (int i = 0; i < size; i++) {
		cout << x[i] << ", " << y[i] << ", " << z[i] << "\n";
  }
}

} // anon namespace


// ============================================================================
// Struct ScalarData
// ============================================================================

ScalarData::ScalarData(int size) : m_size(size) {
	assert(size > 0);

 	x = new float[size];
 	y = new float[size];
 	z = new float[size];
}


ScalarData::~ScalarData() {
 	delete [] z;
 	delete [] y;
 	delete [] x;
}


void ScalarData::init() {
 	init_arrays(x, y, z, m_size);
}


/**
 * @param show_number Max number of items to show.
 *                    If default is set, show all items.
 */
void ScalarData::disp(int show_number) {
	assert(show_number <= m_size);  // disp() trying to show more items than present
	if (show_number == -1) show_number = m_size;

 	disp_arrays(x, y, z, show_number);
}


// ============================================================================
// Main routine
// ============================================================================

void run_scalar_kernel() {
  int size = settings.num_vertices;
	bool have_stl = (stl_num_coords() > 0);

	if (have_stl) {
  	size = stl_num_coords();
	}

	ScalarData data(size);

	if (have_stl) {
		stl_load_data(data);
	} else {
		data.init();
	}

	data.disp(20);

  if (!settings.compile_only) {
    Timer timer;  // Time the run only
    rot3D(data.m_size, settings.rot_x, settings.rot_y, settings.rot_z, data.x, data.y, data.z);
    timer.end(!settings.silent);
  }

	data.disp(20);

  if (settings.save_stl) {
		stl_save_data(data, true);
	}
}

