#ifndef _GRU_MMATRIX_H
#define _GRU_MMATRIX_H
#include "common.h"

bool same(qpu::matrix const &lhs, MatrixXf const &rhs, float precision = 0.0f, bool show_max_diff = false);
bool same(qpu::matrix const &lhs, qpu::matrix const &rhs, float precision = 0.0f, bool show_max_diff = false);
qpu::matrix copy_m(MatrixXf const &rhs);
std::string dump(MatrixXf const &m);

class State;

class MMatrix {
public:
  MMatrix() = default;
  MMatrix(int rows, int columns, float val = 0.0f, bool set_Xf = false);
  MMatrix(MMatrix const &rhs);

  void set(MatrixXf const &rhs);
  void set(MMatrix const &rhs);

  int rows() const;
  int cols() const;
  int size() const { return rows()*cols(); }

  void row(int index, MatrixXf const &val);
  void row(int index, MMatrix const &val);
	void move_rows(int step, MMatrix const &rhs);

  MMatrix row(int index) const;

  MatrixXf    const &Xf()  const;
  qpu::matrix const &qpu() const;

  void Xf(MatrixXf const &val)     { m_Xf  = val; }
  void qpu(qpu::matrix const &val) { m_qpu = val; }

  bool same(float precision = 0.0f) const {
    return ::same(m_qpu, m_Xf, precision);
  }

  bool same(MMatrix const &rhs, float precision = 0.0f, bool show_max_diff = false) const;

  std::string dump_dim() const;
  std::string dump() const;
  void eval() { m_Xf.eval(); }

  void operator+=(MMatrix const &rhs);
  void operator-=(MMatrix const &rhs);
  void operator/=(float steps);
  MMatrix operator*(MMatrix const &rhs) const;
  MMatrix operator*(float val) const;
  MMatrix mul_e(MMatrix const &rhs) const;
  MMatrix outer(MMatrix const &rhs) const;
  void outer_add(MMatrix const &lhs, MMatrix const &rhs);
  void outer_add_rows(MMatrix const &lhs, MMatrix const &rhs, float precision);
  void outer_rows(MMatrix const &lhs, MMatrix const &rhs);

  // Application-specific methods
  void back_prop_1(MMatrix const &ds_cur, State const &temp, float precision);
  void back_prop_3(MMatrix const &dsr, State const &temp, float precision = 0.0f);
  void back_prop_4(MMatrix const &ds_cur_bk, State const &temp, float precision);
  void divide_matrix(MMatrix const &gradient, MMatrix const &in_cache);
  void update_E(int index, MatrixXf const &Y, State const &state);

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

  void reset();
};


inline MMatrix operator*(float val, MMatrix const &rhs) {
  return rhs*val;
}



#endif // _GRU_MMATRIX_H
