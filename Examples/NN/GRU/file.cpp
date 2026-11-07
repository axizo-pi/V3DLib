#include "file.h"
#include "global.h"

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
	std::string path = settings::weights_dir + "/" + filename;

  std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!out) {
		std::string buf;
		buf << "Opening outfile failed for file '" << path << "'";
    throw std::runtime_error(buf);
	}

  typename MatrixXf::Index rows=matrix.rows(), cols=matrix.cols();
  out.write((char*) (&rows), sizeof(typename MatrixXf::Index));
  out.write((char*) (&cols), sizeof(typename MatrixXf::Index));
  out.write((char*) matrix.data(), rows * cols * sizeof(typename MatrixXf::Scalar) );
  out.close();
}


void read_binary_matrix(std::string filename, MatrixXf& matrix) {
	std::string path = settings::weights_dir + "/" + filename;

  std::ifstream in(path, std::ios::in | std::ios::binary);
	if (!in) {
		std::string buf;
		buf << "Opening infile failed for file '" << path << "'";
    throw std::runtime_error(buf);
	}

  typename MatrixXf::Index rows=0, cols=0;
  in.read((char*) (&rows),sizeof(typename MatrixXf::Index));
  in.read((char*) (&cols),sizeof(typename MatrixXf::Index));
  matrix.resize(rows, cols);
  in.read( (char *) matrix.data() , rows * cols * sizeof(typename MatrixXf::Scalar) );
  in.close();
}

