#ifndef _GRU_MODEL_H
#define _GRU_MODEL_H
#include "common.h"

class Model {
public:
  MatrixXf U_z;
	MatrixXf U_r;
	MatrixXf U_h;
	MatrixXf W_z;
	MatrixXf W_r;
	MatrixXf W_h;
	MatrixXf V;

	qpu::matrix q_W_h;
	qpu::matrix q_U_r;

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
  MatrixXf z;
  MatrixXf r;
  MatrixXf h;
  MatrixXf O;
  MatrixXf S;

	qpu::vector q_S;
	qpu::vector q_r;
	qpu::vector q_z;
	qpu::vector q_h;

	State(bool do_temp = false) : m_do_temp(do_temp) {}

  void init(int time_steps, int hidden_dim, int output_dim);
	void eval();
	void copy_q();
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
