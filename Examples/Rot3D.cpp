#include <unistd.h>  // sleep()
#include <math.h>
#include <V3DLib.h>
#include "Support/Settings.h"
#include "Support/Timer.h"
#include "Support/debug.h"
#include "Kernels/Rot3D.h"

using namespace V3DLib;
using namespace kernels;
using namespace std;
using KernelType = decltype(rot3D_1);  // All kernel functions except scalar have same prototype


// ============================================================================
// Command line handling
// ============================================================================

std::vector<const char *> const kernel_id = { "2", "3", "1", "1a", "cpu" };  // First is default

CmdParameters params = {
  "Rot3D\n"
  "\n"
  "Rotates a number of vectors by a given angle.\n"
  "The kernel is IO-bound, the data transfer time dominates over the calculation during execution.\n"
  "It is therefore an indicator of data transfer speed, "
  "and is used for performance comparisons of platforms and configurations.\n",
  {{
    "Kernel",
    "-k=",
    kernel_id,
    "Select the kernel to use"
  }, {
    "Display Results",
    "-d",
    ParamType::NONE,
    "Show the results of the calculations"
  }, {
    "Number of vertices",
    {"-v=", "-vertices="},
    ParamType::POSITIVE_INTEGER,
    "Number of vertices to rotate",
    1920  // was 192000
  }, {
    "Show Info",
    {"info"},
    ParamType::NONE,
    "Show extended information for this application, in particular the calculation used.\n"
		"The actual computation will not run."
  }, {
    "Rotate X",
    {"-rotate_x=", "-rx="},
    ParamType::FLOAT,
    "Rotate around the x-axis by the given angle.\n"
		"The input value is a multiple of PI",
		0.0f
  }, {
    "Rotate Y",
    {"-rotate_y=", "-ry="},
    ParamType::FLOAT,
    "Rotate around the y-axis by the given angle.\n"
		"The input value is a multiple of PI",
		0.0f
  }, {
    "Rotate Z",
    {"-rotate_z=", "-rz="},
    ParamType::FLOAT,
    "Rotate around the z-axis by the given angle.\n"
		"The input value is a multiple of PI",
		0.0f
  }}
};


void info() {
	// Source: https://en.wikipedia.org/wiki/Rotation_matrix 

	std::string msg = "\n"
"There are three rotations allowed on the command line, namely:\n"
"\n"
"  - clockwise around the x axis (param rotate_x)\n"
"  - clockwise around the y axis (param rotate_y)\n"
"  - clockwise around the z axis (param rotate_z)\n"
"\n"
"If you're looking down at the rotation axis, the rotation will be counter-clockwise.\n"
"The following rotation matrices are used:\n"
"\n"
"           |   1       0       0     |\n"
"   Rx(θ) = |   0      cos(θ) -sin(θ) |\n"
"           |   0      sin(θ)  cos(θ) |\n"
"\n"	
"           |  cos(θ)   0      sin(θ) |\n"
"   Ry(θ) = |   0       1       0     |\n"
"           | -sin(θ)   0      cos(θ) |\n"
"\n"	
"           |  cos(θ) -sin(θ)   0     |\n"
"   Rz(θ) = |  sin(θ)  cos(θ)   0     |\n"
"           |   0       0       1     |\n"
"\n"	
"All three rotations can be specified simultaneously on the command line. "
"The rotations are performed in the order of axes x, y, z.\n"	
"\n"	
	;

	cout << msg;
}


struct Rot3DSettings : public Settings {
  const float THETA = (float) 3.14159;  // Rotation angle in radians

  int   kernel;
  bool  show_results;
  int   num_vertices;

	bool  show_info;
	float rot_x;
	float rot_y;
	float rot_z;

  Rot3DSettings() : Settings(&params, true) {}

  bool init_params() override {
    auto const &p = parameters();

    kernel       = p["Kernel"]->get_int_value();
    show_results = p["Display Results"]->get_bool_value();
    num_vertices = p["Number of vertices"]->get_int_value();

    show_info    = p["Show Info"]->get_bool_value();
    rot_x        = p["Rotate X"]->get_float_value();
    rot_y        = p["Rotate Y"]->get_float_value();
    rot_z        = p["Rotate Z"]->get_float_value();

    if (num_vertices % 16 != 0) {
      cout << "ERROR: Number of vertices must be a multiple of 16.\n";
      return false;
    }

		if (show_info) info(); 

    return true;
  }
} settings;



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

	// Add some reasonably random stuff
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

// ============================================================================
// Local functions
// ============================================================================

template<typename Arr>
void init_arrays(Arr &x, Arr &y) {
  for (int i = 0; i < settings.num_vertices; i++) {
    x[i] = (float) i;
    y[i] = (float) i;
  }
}


template<typename Arr>
void disp_arrays(Arr &x, Arr &y) {
  if (settings.show_results) {
    for (int i = 0; i < settings.num_vertices; i++)
      printf("%f %f\n", x[i], y[i]);
  }
}


void run_qpu_kernel(KernelType &kernel) {
  auto k = compile(kernel, settings);  // Construct kernel
  k.setNumQPUs(settings.num_qpus);

  // Allocate and initialise arrays shared between ARM and GPU
  Float::Array x(settings.num_vertices), y(settings.num_vertices);
  init_arrays(x, y);

  Timer timer;
  k.load(settings.num_vertices, cosf(settings.THETA), sinf(settings.THETA), &x, &y).run();
  timer.end(!settings.silent);

  disp_arrays(x, y);
}


void run_qpu_kernel_3() {
  auto k = compile(rot3D_3_decorator(settings.num_vertices, settings.num_qpus), settings);
  k.setNumQPUs(settings.num_qpus);

  // Allocate and initialise arrays shared between ARM and GPU
  Float::Array x(settings.num_vertices), y(settings.num_vertices);
  init_arrays(x, y);

  Timer timer;
  k.load(cosf(settings.THETA), sinf(settings.THETA), &x, &y).run();
  timer.end(!settings.silent);

  disp_arrays(x, y);
}


/**
 * Run a kernel as specified by the passed kernel index
 */
void run_kernel(int kernel_index) {
  switch (kernel_index) {
    case 0: run_qpu_kernel(rot3D_2);  break;  
    case 1: run_qpu_kernel_3();       break;  
    case 2: run_qpu_kernel(rot3D_1);  break;  
    case 3: run_qpu_kernel(rot3D_1a); break;  
    case 4: run_scalar_kernel();      break;
  }

  auto name = kernel_id[kernel_index];

  if (!settings.silent) {
    cout << "Ran kernel '" << name << "' "
		     << "with " << settings.num_qpus << " QPU's.\n";
  }
}


// ============================================================================
// Main
// ============================================================================

int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

	if (!settings.show_info) {
  	run_kernel(settings.kernel);
	}
  return 0;
}
