#ifndef _GRU_MMATRIX_H
#define _GRU_MMATRIX_H
#include "common.h"

class Model;

bool same(qpu::matrix const &lhs, MatrixXf const &rhs, float precision = 0.0f, int bit_diff = -1, bool show_max_diff = false);
bool same(qpu::matrix const &lhs, qpu::matrix const &rhs, float precision = 0.0f, int bit_diff = -1, bool show_max_diff = false);
qpu::matrix copy_m(MatrixXf const &rhs);
std::string dump(MatrixXf const &m);

class State;

class MMatrix {
public:
  MMatrix();
  MMatrix(int rows, int columns, float val = 0.0f, bool set_Xf = false);
  MMatrix(MMatrix const &rhs);
  void resize(int rows, int columns, float val = 0.0f);

  void set(MatrixXf const &rhs, bool set_qpu = false);
  void set(MMatrix const &rhs);

  int rows() const;
  int cols() const;
  int size() const { return rows()*cols(); }

  void row(int index, MatrixXf const &val);
  void row(int index, MMatrix const &val);
  void move_rows(int step, MMatrix const &rhs);
  MMatrix transpose() const;

  MMatrix row(int index) const;

  MatrixXf    const &Xf()  const;
  qpu::matrix const &qpu() const;

  void Xf(MatrixXf const &val)     { m_Xf  = val; }
  void qpu(qpu::matrix const &val) { m_qpu = val; }

  bool same(float precision = 0.0f) const;
  bool same_b(int bit_diff = 0) const;
  bool same(MatrixXf const &rhs, float precision = 0.0f, bool show_max_diff = false) const;
  bool same(MMatrix const &rhs, float precision = 0.0f, bool show_max_diff = false) const;
  bool same_b(MMatrix const &rhs, int bit_diff = -1, bool show_max_diff = false) const;
  bool same_b(MatrixXf const &rhs, int bit_diff = -1, bool show_max_diff = false) const;

  std::string dump_dim() const;
  std::string dump() const;
  void eval() { m_Xf.eval(); }

  MMatrix operator+(MMatrix const &rhs) const;
  MMatrix operator-(MMatrix const &rhs) const;
  void operator+=(MMatrix const &rhs);
  void operator-=(MMatrix const &rhs);
  void operator/=(float steps);
	MMatrix operator/(float steps);
  MMatrix operator*(MMatrix const &rhs) const;
  MMatrix mul_t(MMatrix const &rhs) const;
  MMatrix operator*(float val) const;
  MMatrix mul_e(MMatrix const &rhs) const;
  MMatrix div_e(MMatrix const &rhs) const;
  MMatrix tanh() const;
  MMatrix sigmoid() const;
  MMatrix outer(MMatrix const &rhs) const;
  void outer_add(MMatrix const &lhs, MMatrix const &rhs);
  void outer_add_rows(MMatrix const &lhs, MMatrix const &rhs);
  void outer_rows(MMatrix const &lhs, MMatrix const &rhs);
  MMatrix max_row() const;
  MMatrix sum_row() const;
	float sum() const;
  void softmax();
  MMatrix ln() const;

  // Application-specific methods
  void back_prop_1(MMatrix const &ds_cur, State const &temp, float precision);
  void back_prop_3(MMatrix const &dsr, State const &temp, float precision = 0.0f);
  void back_prop_4(MMatrix const &ds_cur_bk, State const &temp, float precision);
  void divide_matrix(MMatrix const &gradient, MMatrix const &in_cache);
  MMatrix calc_E(MMatrix const &Y, MMatrix const &O) const;
  void col_E(int index, MMatrix const &rhs);
	MMatrix forward_1(Model &m, MMatrix &S);
	MMatrix forward_2(Model &m, MMatrix &S);
	MMatrix forward_3(Model &m, MMatrix &S, MMatrix const &r_row);
	MMatrix forward_4(MMatrix &S, MMatrix const &h_row);
	MMatrix forward_5() const;

  void set_decay(float decay, MMatrix const &rhs);

private:  
  mutable MatrixXf    m_Xf;
  mutable qpu::matrix m_qpu;

  mutable bool m_using_Xf  = false;
  mutable bool m_using_qpu = false;

  void need_fields(bool need_XF, bool need_qpu) const;
  void used_fields(bool in_XF, bool in_qpu) { m_using_Xf = in_XF; m_using_qpu = in_qpu; }

  void copy_row(int from_index, int to_index, MMatrix const &val);
  void copy_block(MMatrix const &rhs, int from_offset, int to_offset, int in_size);
  bool same_intern(MMatrix const &rhs, float precision, int bit_diff, bool show_max_diff) const;

  void reset();

  MMatrix invert() const;
};


inline MMatrix operator*(float val, MMatrix const &rhs) {
  return rhs*val;
}


MMatrix remove_last_rows(int num, MMatrix const &rhs);

#endif // _GRU_MMATRIX_H
