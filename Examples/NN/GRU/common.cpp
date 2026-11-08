#include "common.h"

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


std::string dump_dim(MatrixXf const &m) { return MatrixXfAdapter(m).dump_dim(); }
std::string dump(MatrixXf const &m)     { return matrix_dump(MatrixXfAdapter(m), false); }
bool        is_zero(MatrixXf const &m)  { return MatrixXfAdapter(m).is_zero(); }
