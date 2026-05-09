#ifndef _GRU_MODEL_H
#define _GRU_MODEL_H
#include "mmatrix.h"


class Model {
public:
  MMatrix  U_z;
  MMatrix  U_r;
  MMatrix  U_h;
  MMatrix  W_z;
  MMatrix  W_r;
  MMatrix  W_h;
  MMatrix  V;

  void read(std::string const &epoch, std::string const &loss);
  void write(int epoch, float loss);
  void init(int input_dim, int hidden_dim, int output_dim);
  void init_zeroes(int input_dim, int hidden_dim, int output_dim, bool do_eval = false);
  void init_ones(int input_dim, int hidden_dim, int output_dim);

  int input_dim()  const { return (int) U_z.rows(); }
  int hidden_dim() const { return (int) U_z.cols(); }
  int output_dim() const { return (int) V.cols(); }

  void grad_div_steps(float steps);
  void cache_decay(float decay, Model &grad);
  void divide(Model &grad, Model &cache);
  void adjust_learning_rate(float learning_rate, Model &rhs);
  void eval();
  std::string dump_dim() const;
};


class State {
public:  
  MatrixXf E;  // Unused in test
  MMatrix z;
  MMatrix r;
  MMatrix h;
  MatrixXf O;
  MMatrix S;

  State(bool do_temp = false) : m_do_temp(do_temp) {}

  void init(int time_steps, int hidden_dim, int output_dim);
  void eval();
  void set_step(int time_step, State &state);

private:
  bool m_do_temp;  
};


void forward_propagation(
  Model &m,
  MatrixXf& X,
  MatrixXf& Y,  // Not used in test
  State &state,
  int time_steps,
  int input_dim,
  int hidden_dim,
  int output_dim,
  bool do_test = false);

#endif //  _GRU_MODEL_H
