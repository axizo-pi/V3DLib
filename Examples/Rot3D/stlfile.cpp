#include "stlfile.h"
#include "stl_int.h"
#include "global/log.h"
#include <vector>

using namespace std;
using namespace Log;

namespace {

vector<openstl::Triangle> triangles;

} // anon namespace


void load_stl(std::string const &filename) {
	if (filename.empty()) return;
	warn << "Have STl file";

	assert(triangles.empty(), "STL triangles already filled");
	if (!stl::read(filename, triangles)) {
    error << "Error: Unable to open file '" << filename << "'";
	}

	warn << "Num triangles: " << triangles.size();
}


/**
 * Triangles are defined by 3 3D-vectors.
 * Therefore the number of coordinatesi (3D) is triple the number of triangles.
 */
int stl_num_coords() {
	return (int) (triangles.size() * 3);
}


void stl_load_data(ScalarData &data) {
	warn << "stl_load_data called";

	assert(!triangles.empty(), "No STL triangles");
	assert(((long unsigned) data.m_size) == (triangles.size()*3), "Size must match");

	// Data in 'data' will be overwritten
	for (unsigned long i = 0; i < triangles.size(); ++i) {
		auto const &t = triangles[i];

		data.x[3*i] = t.v0.x;
		data.y[3*i] = t.v0.y;
		data.z[3*i] = t.v0.z;

		data.x[3*i + 1] = t.v1.x;
		data.y[3*i + 1] = t.v1.y;
		data.z[3*i + 1] = t.v1.z;

		data.x[3*i + 2] = t.v2.x;
		data.y[3*i + 2] = t.v2.y;
		data.z[3*i + 2] = t.v2.z;
	}
}


void stl_save_data(ScalarData &data, bool save_file) {
	warn << "stl_save_data called";

	assert(!triangles.empty(), "No STL triangles");
	assert(((long unsigned) data.m_size) == (triangles.size()*3), "Size must match");

	// Data in triangles list will be overwritten
	for (unsigned long i = 0; i < triangles.size(); ++i) {
		auto &t = triangles[i];

		t.v0.x = data.x[3*i];
		t.v0.y = data.y[3*i]; 
		t.v0.z = data.z[3*i];

		t.v1.x = data.x[3*i + 1];
		t.v1.y = data.y[3*i + 1]; 
		t.v1.z = data.z[3*i + 1];

		t.v2.x = data.x[3*i + 2];
		t.v2.y = data.y[3*i + 2]; 
		t.v2.z = data.z[3*i + 2];
	}

	if (save_file) {
		warn << "stl_save_data saving to file";
		stl::write("out.stl", triangles, false);
	}
}
