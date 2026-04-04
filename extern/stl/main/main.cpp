/**
 * Itsy-bitsy little test program for STL handling.
 */
#include "stl_int.h"

using namespace std;

int main(int arc, char **argv) {
	string filename = "icosahedron_dl.stl";
								 // "Utah_teapot_(solid).stl";

	vector<openstl::Triangle> triangles;
	stl::read(filename, triangles);

	cout << "Num triangles: " << triangles.size() << "\n";

	cout << "First ten triangles:\n";
	for (int i = 0; i < 10; ++i) {
		cout << stl::dump(triangles[i]) << "\n\n";
	}

	stl::write("out.stl", triangles, false);

	return 0;
}
