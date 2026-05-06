#ifndef _GRU_COMMON_H
#define _GRU_COMMON_H
#include "Support/Timer.h"
#include "../Lib/matrix.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"

#include <Eigen/Dense>

#pragma GCC diagnostic pop

#include <string>
#include <fstream>

using namespace Eigen;
using namespace V3DLib;

void write_binary_matrix(std::string filename, const MatrixXf& matrix);
void read_binary_matrix(std::string filename, MatrixXf& matrix);

float sigmoid(float x);
float sigmoid_grad(float x);
float relu(float x);
float relu_grad(float x);
float tanh_activation(float x);
float tanh_grad(float x);
float log_matrix(float x);

void read_input(MatrixXf& X, std::ifstream& inputFile, int time_steps);
void read_output(MatrixXf& Y, std::ifstream& outputFile, int time_steps);

std::string dump_dim(MatrixXf const &m);
qpu::vector copy(MatrixXf const &rhs);

#endif // _GRU_COMMON_H
