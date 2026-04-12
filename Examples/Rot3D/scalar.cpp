#include "scalar.h"
#include "settings.h"
#include "Support/Timer.h"
#include "Kernels/Rot3D.h"
#include <iostream>
#include "stlfile.h"

using namespace std;
using namespace V3DLib;
using namespace kernels;

	
// ============================================================================
// Struct ScalarData
// ============================================================================

struct ScalarData : public StlData {
	float *x = nullptr;
	float *y = nullptr;
	float *z = nullptr;

	ScalarData(int in_size);
	~ScalarData();

	float xc(int index) const override { return x[index]; }
	float yc(int index) const override { return y[index]; }
	float zc(int index) const override { return z[index]; }

	void xc(int index, float rhs) override { x[index] = rhs; }
	void yc(int index, float rhs) override { y[index] = rhs; }
	void zc(int index, float rhs) override { z[index] = rhs; }
};

ScalarData::ScalarData(int in_size) : StlData(in_size) {
	// size will change if an stl file is loaded
 	x = new float[size()];
 	y = new float[size()];
 	z = new float[size()];
}


ScalarData::~ScalarData() {
 	delete [] z;
 	delete [] y;
 	delete [] x;
}




// ============================================================================
// Main routine
// ============================================================================

void run_scalar_kernel() {
  int size = settings.num_vertices;
	ScalarData data(size);
	data.init();
	data.disp("Data pre");

  if (!settings.compile_only) {
    Timer timer;  // Time the run only
    scalar_rot3D(data.size(), settings.rot_x, settings.rot_y, settings.rot_z, data.x, data.y, data.z);
    timer.end(!settings.silent);
  }

	data.disp("Data post");

  if (settings.save_stl) {
		stl_save_data(data, true);
	}
}

