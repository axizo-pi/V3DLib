#include "stlfile.h"
#include "stl_int.h"
#include "settings.h"
#include "global/log.h"
#include <vector>

using namespace std;
using namespace Log;

namespace {

vector<openstl::Triangle> triangles;

} // anon namespace


StlData::StlData(int size) : m_size(size) {
	assert(size > 0, "size must be positive");

	bool have_stl = (stl_num_coords() > 0);

	if (have_stl) {
		warn << "StlData ctor stl present, overriding size";
  	m_size = stl_num_coords();
	}
}


void StlData::init_v(int index, float in_x, float in_y, float in_z) {
		xc(index, in_x);
		yc(index, in_y);
		zc(index, in_z);
}


void StlData::init() {
	bool have_stl = (stl_num_coords() > 0);
	if (have_stl) {
		warn << "StlData::init stl present, loading that instead";
		stl_load_data(*this);
		return;
	}

	assert(m_size % 16 == 0, "size must be multiple of 16");
	assert(m_size >= 16, "size must be at least 16");

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
  for (int i = 9; i < m_size; i++) {
    float x_in = (float) ((i +2)% 4);
    float y_in = (float) (i % 4);
    float z_in = 1;

    x_in *= (float) (i + 1);
    y_in *= (float) (i + 1);
    z_in *= (float) (i + 1);

		xc(i, x_in);
		yc(i, y_in);
		zc(i, z_in);
  }
}


void StlData::disp_arrays(int size) const {
  if (!settings.show_results) return;

  for (int i = 0; i < size; i++) {
		std::cout << xc(i) << ", " << yc(i) << ", " << zc(i) << "\n";
  }
}


/**
 * @param show_number Max number of items to show.
 *                    If default is set, show all items.
 */
void StlData::disp(int show_number) const {
	assert(show_number <= m_size, "disp() trying to show more items than present");
	if (show_number == -1) show_number = m_size;

 	disp_arrays(show_number);
}


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
	//warn << "stl_num_coords triangles size: " << triangles.size();
	return (int) (triangles.size() * 3);
}


void stl_load_data(StlData &data) {
	warn << "stl_load_data called";

	assert(!triangles.empty(), "No STL triangles");
	assert(((long unsigned) data.size()) == (triangles.size()*3), "Size must match");

	// Data in 'data' will be overwritten
	for (int i = 0; i < (int) triangles.size(); ++i) {
		auto const &t = triangles[i];

		data.init_v(3*i    , t.v0.x, t.v0.y, t.v0.z);
		data.init_v(3*i + 1, t.v1.x, t.v1.y, t.v1.z);
		data.init_v(3*i + 2, t.v2.x, t.v2.y, t.v2.z);
	}
}


void stl_save_data(StlData &data, bool save_file) {
	warn << "stl_save_data called";

	assert(!triangles.empty(), "No STL triangles");
	assert(((long unsigned) data.size()) == (triangles.size()*3), "Size must match");

	// Data in triangles list will be overwritten
	for (int i = 0; i < (int) triangles.size(); ++i) {
		auto &t = triangles[i];

		t.v0.x = data.xc(3*i);
		t.v0.y = data.yc(3*i); 
		t.v0.z = data.zc(3*i);

		t.v1.x = data.xc(3*i + 1);
		t.v1.y = data.yc(3*i + 1); 
		t.v1.z = data.zc(3*i + 1);

		t.v2.x = data.xc(3*i + 2);
		t.v2.y = data.yc(3*i + 2); 
		t.v2.z = data.zc(3*i + 2);
	}

	if (save_file) {
		warn << "stl_save_data saving to file";
		stl::write("out.stl", triangles, false);
	}
}
