/**
 * Source: https://github.com/Innoptech/OpenSTL
 *
 * Only downloaded the include file.
 *
 * Code taken from examples in README.
 */
#include "stl_int.h"

namespace {

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


} // anon namespace


//////////////////////////////////////////////////////////////
// Public Functions
//////////////////////////////////////////////////////////////

namespace stl {

/**
 * Read STL from file
 *
 * @return true if file open succeeded, false otherwise
 */
bool read(std::string const &filename, std::vector<openstl::Triangle> &triangles) {

	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
    //std::cerr << "Error: Unable to open file '" << filename << "'" << std::endl;
		return false;
	}

	// Deserialize the triangles in either binary or ASCII format
	triangles = openstl::deserializeStl(file);
	file.close();

	return true;
}


/**
 * Write STL to a file
 */
void write(std::string const &filename, std::vector<openstl::Triangle> const &triangles, bool do_binary) {
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


std::string const dump(openstl::Triangle const &t) {
	std::stringstream buf;

	buf << "  v0: " << t.v0.x << ", " << t.v0.y << ", " << t.v0.z << "\n"
			<< "  v1: " << t.v1.x << ", " << t.v1.y << ", " << t.v1.z << "\n"
			<< "  v2: " << t.v2.x << ", " << t.v2.y << ", " << t.v2.z;

	if (t.attribute_byte_count > 0) {
  	buf << "\n   attribute_byte_count: " << t.attribute_byte_count;
	}

	return buf.str();
}

} // namespace stl
