#ifndef _INCLUDE_RNNSUPPORT_MATRIX_H
#define _INCLUDE_RNNSUPPORT_MATRIX_H
#include "kernels.h"
#include "Support/basics.h"
#include "Support/Settings.h"

namespace qpu {

using namespace Log;
using namespace V3DLib;


////////////////////////////////////////////////
// Class matrix
////////////////////////////////////////////////

/**
 * Width must be in multiples of 16.
 *
 * Case `columns x rows = 16*16 x 16`:
 * - has been shown to work
 * - are dimension which I consider to efficiently use the QPU
 */
struct matrix {
  matrix(int rows = 0, int columns = 0);
  matrix(matrix const &rhs);

  int columns() const  { return m_columns; }
  int rows() const     { return m_rows; }

  Float::Array &operator &();
  Float::Array &arr();
  Float::Array const &arr() const;
  void rand();
  void set(float init_val);
  void set(Float::Array const &rhs);
  void frand();

  matrix &operator=(matrix const &rhs);
  matrix operator-(matrix const &rhs);
  matrix &operator-=(matrix const &rhs);
  matrix operator+(matrix const &rhs);
  matrix &operator+=(matrix const &rhs);
  matrix operator*(float rhs) const;
  matrix mul(matrix const &rhs) const;
  matrix mul_t(matrix const &rhs) const;
  matrix sigmoid_derivative(matrix const &rhs);
  matrix transpose() const;

  std::string dump_dim() const;
  std::string dump(bool output_int = false) const;
  inline float &at(int i, int j)       { return (*m_arr)[i*m_columns + j]; }
  inline float  at(int i, int j) const { return (*m_arr)[i*m_columns + j]; }

protected:
  void rows(int rhs)    { m_rows = rhs; }
  void columns(int rhs) { m_columns = rhs; }

  void transfer(matrix const &rhs);
  matrix mul_elem(matrix const &rhs) const;

  std::shared_ptr<Float::Array> m_arr;

private:
  int m_rows     = 0;
  int m_columns  = 0;

  static BaseKernel *m_mult_vec;
  static BaseKernel *m_mult_vec_transposed;

  void init_static();
};


matrix operator*(float scalar, matrix const &mat);


////////////////////////////////////////////////
// Class vector 
////////////////////////////////////////////////

/**
 * Size must be a multiple of 16
 */
struct vector : public matrix {
	using Parent = matrix;

  vector(vector &rhs);
  explicit vector(matrix rhs);
  vector(int rows = 0, float val = 0.0f);

  void set(float *rhs, int in_size);
  void set(float init_val);
  float &operator[](int index);
  float operator[](int index) const;
  int size() const;

  vector operator-(vector const &rhs);
  vector operator+(vector const &rhs);
  vector &operator+=(vector const &rhs) { *this = *this + rhs;  return *this; }
  vector mul(vector const &rhs);
  vector &operator=(matrix const &rhs);
  vector &operator=(vector const &rhs);
  matrix outer(matrix const &rhs) const;
  vector sigmoid(vector const &bias);
  vector dsigmoid();
  vector tanh();
  vector dtanh();
  void clip(float clip_value);
  static BaseKernel &op_kernel();

  //std::string dump_dim() const { return matrix::dump_dim(); }
  std::string dump(bool output_int = false) const;

private:
  bool empty() const { return m_arr == nullptr; }

  // TODO: should probably clean this up.
  //       ptr's not cleaned up on exit, better would be shared or unique ptr.
  // For now, it works
  static BaseKernel *m_sub;
  static BaseKernel *m_add;
  static BaseKernel *m_op;
  static BaseKernel *m_sigmoid;
  static BaseKernel *m_dsigmoid;
  static BaseKernel *m_tanh;
  static BaseKernel *m_dtanh;
  static BaseKernel *m_clip;

  static void init_static();
};


vector operator*(matrix const &lhs, vector const &rhs);

bool check_precision(float lhs, float rhs, float precision);
bool same(qpu::vector const &lhs, qpu::vector const &rhs, float precision = 0.0f);

} // namespace qpu

#endif // _INCLUDE_RNNSUPPORT_MATRIX_H
