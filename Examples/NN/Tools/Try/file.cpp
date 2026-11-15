#include "file.h"
#include "global/global.h"
#include <fstream>

using namespace Eigen;

/**
 * \file
 *
 * File operations for this example program
 */

namespace {

std::vector<int> load_file_vector(std::string filename) {
  std::vector<int> ret;

  filename.replace(filename.end() - 3, filename.end(), "bin");
  //warn << "filename: " << filename;

  std::ifstream file_input(filename, std::ios::binary);
  assert(!file_input.fail());

  file_input.seekg(0, std::ios::beg);
  uint32_t a = 0;

  while(!file_input.eof()) {
    file_input.read((char*)&a, sizeof(uint32_t));
    ret.push_back((int) a);
  }

  file_input.close();

  return ret;
}

}  // anon namespace


void load_x_y(
  std::string filename_input,
  std::string filename_output,
  std::vector<int> &x_input,
  std::vector<int> &x_output
) {
  assert(x_input.empty());
  assert(x_output.empty());

  x_input  = load_file_vector(filename_input);
  x_output = load_file_vector(filename_output);

  assert(!x_input.empty());
  assert(!x_output.empty());
}


void write_binary_matrix(std::string filename, const MatrixXf& matrix){
  std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out) {
    std::string buf;
    buf << "Opening outfile failed for file '" << filename << "'";
    throw std::runtime_error(buf);
  }

  typename MatrixXf::Index rows=matrix.rows(), cols=matrix.cols();
  out.write((char*) (&rows), sizeof(typename MatrixXf::Index));
  out.write((char*) (&cols), sizeof(typename MatrixXf::Index));
  out.write((char*) matrix.data(), rows * cols * sizeof(typename MatrixXf::Scalar) );
  out.close();
}


void read_binary_matrix(std::string filename, MatrixXf& matrix) {
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in) {
    std::string buf;
    buf << "Opening infile failed for file '" << filename << "'";
    throw std::runtime_error(buf);
  }

  typename MatrixXf::Index rows=0, cols=0;
  in.read((char*) (&rows),sizeof(typename MatrixXf::Index));
  in.read((char*) (&cols),sizeof(typename MatrixXf::Index));
  matrix.resize(rows, cols);
  in.read( (char *) matrix.data() , rows * cols * sizeof(typename MatrixXf::Scalar) );
  in.close();
}

namespace {

  char mapping_64[] = {
		'\n', ' ', '!', '"', '#', '$', '%', '&',
		 '`', '(', ')', '+', ',', '-', '.', '/',
		 '0', '1', '2', '3', '4', '5', '6', '7',
		 '8', '9', ':', ';', '=', '?', '@', '[',
		 ']', '_', 'a', 'b', 'c', 'd', 'e', 'f',
		 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		 'w', 'x', 'y', 'z', '{', '|', '}', '~'
	};

	bool s_map_initialized = false;
  std::map<char, int> s_char_to_int;


void init_char_to_int_map() {
	if (s_map_initialized) return;  // Lazy load

  for (int i = 0; i < 63; ++i) {
    s_char_to_int.insert(std::pair<char, int>(mapping_64[i], i));
  }

	s_map_initialized = true;
}  

}  // anon namespace


std::map<char, int> const &char_to_int_map() {
	init_char_to_int_map();
  return s_char_to_int;
}


std::string index_to_s(std::vector<int> const &predictions) {
  std::string buf;

  for (auto p : predictions) {
    char c = mapping_64[p];
    std::string buf2;
    if (p == 0) {
      buf2 = "\\n";
    } else {
      buf2 += c;
    }

    buf << buf2;
  }

  return buf;
}


std::vector<int> s_to_index(std::string const &s) {
	std::vector<int> ret;

  for(auto c : s) {
    ret.push_back(char_to_int_map().find(c)->second);
  }

	return ret;
}
