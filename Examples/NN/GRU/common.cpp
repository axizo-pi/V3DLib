#include "common.h"

void write_binary_matrix(std::string filename, const MatrixXf& matrix){
  std::ofstream out(filename, std::ios::out | std::ios::binary | std::ios::trunc);
  typename MatrixXf::Index rows=matrix.rows(), cols=matrix.cols();
  out.write((char*) (&rows), sizeof(typename MatrixXf::Index));
  out.write((char*) (&cols), sizeof(typename MatrixXf::Index));
  out.write((char*) matrix.data(), rows * cols * sizeof(typename MatrixXf::Scalar) );
  out.close();
}


void read_binary_matrix(std::string filename, MatrixXf& matrix){
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  typename MatrixXf::Index rows=0, cols=0;
  in.read((char*) (&rows),sizeof(typename MatrixXf::Index));
  in.read((char*) (&cols),sizeof(typename MatrixXf::Index));
  matrix.resize(rows, cols);
  in.read( (char *) matrix.data() , rows * cols * sizeof(typename MatrixXf::Scalar) );
  in.close();
}


float sigmoid(float x)      { return 1.0f / (1.0f + (float) exp(-x)); }
float sigmoid_grad(float x) { return (1 - x) * x; }


float relu(float x) {
  if (x <= 0)
    return 0.001f * x;
  else
    return x;
}


float relu_grad(float x) {
  if (x <= 0)
    return 0.001f;
  else
    return 1;
}


/* Tanh activation function */
float tanh_activation(float x) {
  return (float) tanh(x);
}

/* Tanh of sigmoid function */
float tanh_grad(float x) {
  return 1.0f - (x * x);
}


float log_matrix(float x) {
  if (x <= 0)
    return 0.0f;
  else
    return (float) log(x);
}


void read_input(MatrixXf& X, std::ifstream& inputFile, int time_steps) {
  /*
   * Second while loop depends on encoding scheme - for 27 character encoding
   */
  int count = 0;
  int n;
  while (!inputFile.eof() && count < time_steps) {
    inputFile >> n;
    X(count, int(n)) = 1;
    count++;
  }

  while (count < time_steps) {
    X(count, 0) = 1;
    count ++;
  }
  X.eval();
}


void read_output(MatrixXf& Y, std::ifstream& outputFile, int time_steps) {
  int count = 0;
  int n;
  while (!outputFile.eof() && count < time_steps) {
    outputFile >> n;
    Y(count, int(n)) = 1;
    count++;
  }
  while (count < time_steps) {
    Y(count, 0) = 1;
    count ++;
  }
  Y.eval();
}


std::string dump_dim(MatrixXf const &m) {
  std::string buf;
  buf << "(rows,cols) = (" << m.rows() << ", " << m.cols() << ")";
  return buf;
}


qpu::vector copy(MatrixXf const &rhs) {
  if (rhs.rows() != 1) {
    breakpoint;
    warn << "copy(MatrixXf) failed due to dimensions, dim rhs: " << dump_dim(rhs) << thrw;
  }
  assert(rhs.cols() % 16 == 0);

  //int height = resize(rhs.size());
  //auto width = resize(rhs[0].size());
  int width = (int) rhs.cols();

  qpu::vector ret(width);
  ret.set(0.0f);

  for (int i = 0; i < rhs.cols(); i++) {
    ret.at(0, i) = rhs(0, i);
  }

  return ret;
}
