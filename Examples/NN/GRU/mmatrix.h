#ifndef _GRU_MMATRIX_H
#define _GRU_MMATRIX_H
#include "common.h"

bool same(qpu::matrix const &lhs, MatrixXf const &rhs, float precision = 0.0f);
bool same(qpu::matrix const &lhs, qpu::matrix const &rhs, float precision = 0.0f);
qpu::matrix copy_m(MatrixXf const &rhs);
std::string dump(MatrixXf const &m);

class State;

class MMatrix {
public:
  MMatrix() = default;
  MMatrix(int rows, int columns, float val = 0.0f);

  void set(MatrixXf const &rhs);
  void sync_qpu();

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
  void operator-=(MMatrix const &rhs);
  void operator/=(float steps);
  MMatrix operator*(MMatrix const &rhs) const;
  MMatrix operator*(float val) const;
  void mul_e(MMatrix const &rhs, State const &temp);
  MMatrix mul_e(MMatrix const &rhs);
  MMatrix outer(MMatrix const &rhs) const;
  void outer_add(MMatrix const &lhs, MMatrix const &rhs);

  // Application-specific methods
  void back_prop_1(MMatrix const &ds_cur, State const &temp);
  void back_prop_2(State const &temp, MMatrix const &dreluInput_h, float precision);
  void back_prop_3(MMatrix const &dsr, State const &temp, float precision);
  void back_prop_4(MMatrix const &ds_cur_bk, State const &temp, float precision);
  void divide_matrix(MMatrix const &gradient, MMatrix const &in_cache);
  void update_E(int index, MatrixXf const &Y, State const &state);

  void set_decay(float decay, MMatrix const &rhs);

private:  
  MatrixXf    m_Xf;
  qpu::matrix m_qpu;
};


inline MMatrix operator*(float val, MMatrix const &rhs) {
  return rhs*val;
}


#endif // _GRU_MMATRIX_H
