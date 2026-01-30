/**
 * Source: https://github.com/Innoptech/OpenSTL
 *
 * Only downloaded the include file.
 *
 * Code taken from examples in README.
 */
#include "stl.h"

/**
 * Read STL from file
 */
void read_stl(std::string const &filename, std::vector<openstl::Triangle> &triangles) {

	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
    std::cerr << "Error: Unable to open file '" << filename << "'" << std::endl;
	}

	// Deserialize the triangles in either binary or ASCII format
	triangles = openstl::deserializeStl(file);
	file.close();
}

/**
 * Write STL to a file
 */
void write_stl(std::string const &filename, std::vector<openstl::Triangle> const &triangles, bool do_binary) {

	std::ofstream file(filename, std::ios::binary);
	if (!file.is_open()) {
    std::cerr << "Error: Unable to open file '" << filename << "'" << std::endl;
	}

	openstl::serialize(triangles, file, do_binary?(openstl::StlFormat::Binary):(openstl::StlFormat::ASCII));

	if (file.fail()) {
    std::cerr << "Error: Failed to write to file " << filename << std::endl;
	} else {
    std::cout << "File " << filename << " has been successfully written." << std::endl;
	}
	file.close();
}


/**
 * Serialize STL to a stream.
 *
 * Probably will not be used, not tested just compiled.
 * Taken over for completeness.
 */
void serialize_stl() {
	std::stringstream ss;

	std::vector<openstl::Triangle> originalTriangles{}; // User triangles
	openstl::serialize(originalTriangles, ss, openstl::StlFormat::Binary); // Or StlFormat::ASCII
}


/**
 * Convert Triangles to Vertices and Faces
 *
 * Probably will not be used, not tested just compiled.
 * Taken over for completeness.
 */
void triangles_to_vertices_and_faces() {
	using namespace openstl;

	std::vector triangles = {
    //        normal,             vertices 0,         vertices 1,        vertices 2
    Triangle{{0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f}},
    Triangle{{0.0f, 0.0f, 1.0f}, {2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f}, {4.0f, 4.0f, 4.0f}}
	};

	const auto& [vertices, faces] = convertToVerticesAndFaces(triangles);
}


/**
 * Convert Vertices and Faces to Triangles
 *
 * Probably will not be used, not tested just compiled.
 * Taken over for completeness.
 */
void vertices_and_faces_to_triangles() {
	using namespace openstl;

	std::vector vertices = {
    Vec3{0.0f, 0.0f, 0.0f}, Vec3{1.0f, 1.0f, 1.0f}, Vec3{2.0f, 2.0f, 2.0f}, Vec3{3.0f, 3.0f, 3.0f}
	};
	std::vector<Face> faces = {
    {0, 1, 2}, {3, 1, 2}
	};

	const auto& triangles = convertToTriangles(vertices, faces);
}


int main(int arc, char **argv) {
	std::string filename = // "icosahedron_dl.stl";
												 "Utah_teapot_(solid).stl";

	std::vector<openstl::Triangle> triangles;
	read_stl(filename, triangles);

	std::cout << "Num triangles: " << triangles.size() << "\n";

	write_stl("teapot.stl", triangles, false);

	return 0;
}
