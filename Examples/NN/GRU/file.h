#ifndef _GRU_FILE_H
#define _GRU_FILE_H
#include <Eigen/Dense>
#include <vector>
#include <string>

void load_x_y(
  std::string filename_input,
  std::string filename_output,
  std::vector<int> &x_input,
  std::vector<int> &x_output
);

void write_binary_matrix(std::string filename, const Eigen::MatrixXf& matrix);
void read_binary_matrix(std::string filename, Eigen::MatrixXf& matrix);

#endif // _GRU_FILE_H
