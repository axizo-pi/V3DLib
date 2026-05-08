#ifndef _GRU_MODEL_H
#define _GRU_MODEL_H
#include "common.h"

bool same(qpu::matrix const &lhs, MatrixXf const &rhs, float precision = 0.0f);
qpu::matrix copy_m(MatrixXf const &rhs);

class State;

class MMatrix {
public:	

	void init_zeroes(int dim);
	void init_ones(int dim);
	void set(MatrixXf const &rhs);

	int rows() const { return (int) m_Xf.rows(); }
	int cols() const { return (int) m_Xf.cols(); }

	void row(int index, MatrixXf const &val) {
    m_Xf.row(index) = val;
    m_Xf.eval();
	}

	MatrixXf row(int index) const {
		return m_Xf.row(index);
	}

	MatrixXf    const &Xf()  const { return m_Xf; }
	qpu::matrix const &qpu() const { return m_qpu; }

	void Xf(MatrixXf const &val)     { m_Xf  = val; }
	void qpu(qpu::matrix const &val) { m_qpu = val; }

	bool same(float precision = 0.0f) const {
		return ::same(m_qpu, m_Xf, precision);
	}

	bool same(MMatrix const &rhs, float precision = 0.0f) const;

	std::string dump_dim() const;
	void eval() { m_Xf.eval(); }

	void operator+=(MMatrix const &rhs);
	void operator/=(float steps);
	MMatrix operator*(MMatrix const &rhs);
	void mul_e(MMatrix const &rhs, State const &temp);
	MMatrix mul_e(MMatrix const &rhs);

	MMatrix outer(MMatrix const &rhs);
	void back_prop_1(MMatrix const &ds_cur, State const &temp);
	void back_prop_2(State const &temp, MMatrix const &dreluInput_h, float precision);
	void back_prop_3(MMatrix const &dsr, State const &temp);
	void back_prop_4(MMatrix const &ds_cur_bk, State const &temp);

private:	
	MatrixXf    m_Xf;
	qpu::matrix m_qpu;
};


class Model {
public:
  MMatrix  U_z;
	MMatrix  U_r;
	MMatrix  U_h;
	MMatrix  W_z;
	MMatrix  W_r;
	MMatrix  W_h;
	MatrixXf V;

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
