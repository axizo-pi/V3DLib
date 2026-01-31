#include "settings.h"
#include <vector>
#include <iostream>

using namespace std;


// ============================================================================
// Command line handling
// ============================================================================

std::vector<const char *> const kernel_id = { "2", "3", "1", "1a", "cpu" };  // First is default

namespace {

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

} // anon namespace


Rot3DSettings::Rot3DSettings() : Settings(&params, true) {}

bool Rot3DSettings::init_params() {
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


struct Rot3DSettings settings;
