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
  void init_val(int input_dim, int hidden_dim, int output_dim, float val, bool do_eval = false);

  int input_dim()  const { return (int) U_z.rows(); }
  int hidden_dim() const { return (int) U_z.cols(); }
  int output_dim() const { return (int) V.cols(); }

  bool is_zero() const;
  void grad_div_steps(int in_steps);
  void cache_decay(float decay, Model &grad);
  void divide(Model &grad, Model &cache);
  void adjust_learning_rate(float learning_rate, Model &rhs);
  void eval();
  std::string dump_dim() const;
};


class State {
public:  
  MMatrix E;  // Unused in test
  MMatrix z;
  MMatrix r;
  MMatrix h;
  MMatrix O;  // Used in test, train, forward, but not initialized for do_temp = true

  State(bool do_temp = false) : m_do_temp(do_temp) {}

  MMatrix &S();
  MMatrix const &S() const;

  void init(int time_steps, int hidden_dim, int output_dim);
	std::string dump() const;
  void eval();
  void set_step(int time_step, State const &state);
  void move_rows(int step, State const &state);

private:
  bool m_do_temp;

  MMatrix m_S;
};

#endif //  _GRU_MODEL_H
