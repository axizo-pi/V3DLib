#ifndef _INCLUDE_RNNSUPPORT_MATRIX_H
#define _INCLUDE_RNNSUPPORT_MATRIX_H
#include "../Lib/kernels.h"
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
  matrix(bool init_static) : m_rows(0), m_columns(0) {}
  matrix(int rows = 0, int columns = 0);
  matrix(matrix const &rhs);

  void resize(int rows, int columns);
  matrix row(int index) const;

  int columns() const  { return m_columns; }
  int rows() const     { return m_rows; }
  int size() const     { return m_columns*m_rows; }

  bool is_vector() const {
    return
      (m_columns == 1 && m_rows >  1) || 
      (m_columns >  1 && m_rows == 1);
  }

  Float::Array &operator &();
  Float::Array &arr();
  Float::Array const &arr() const;
  void rand();
  void set(float init_val);
  void set(Float::Array const &rhs);
  void frand();

  matrix &operator=(matrix const &rhs);
  matrix operator-(matrix const &rhs) const;
  matrix &operator-=(matrix const &rhs);
  matrix operator+(matrix const &rhs) const;
  matrix &operator+=(matrix const &rhs);
  matrix operator*(float rhs) const;
  matrix mul(matrix const &rhs) const;
  matrix mul_t(matrix const &rhs) const;
  matrix mul_matrix(matrix const &rhs) const;
  matrix mul_matrix_t(matrix const &rhs) const;
  matrix mul_e(matrix const &rhs) const;
  matrix sigmoid_derivative(matrix const &rhs);
  matrix transpose() const;
  matrix outer(matrix const &rhs) const;
  void   outer_add(matrix const &lhs, matrix const &rhs);

  std::string dump_dim() const;
  std::string dump(bool output_int = false) const;
  inline float &at(int i, int j)       { return (*m_arr)[i*m_columns + j]; }
  inline float  at(int i, int j) const { return (*m_arr)[i*m_columns + j]; }

protected:
  void transfer(matrix const &rhs);
  void check_addsub(matrix const &rhs, std::string const &op) const;

  std::shared_ptr<Float::Array> m_arr;

  int m_rows     = 0;
  int m_columns  = 0;

private:
  int m_size = 0;
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
  vector sigmoid(vector const &bias);
  vector dsigmoid() const;
  vector tanh();
  vector dtanh() const;
  void clip(float clip_value);
  static BaseKernel &op_kernel();

  std::string dump(bool output_int = false) const;

private:
  bool empty() const { return m_arr == nullptr; }
  void transpose();
};


vector operator*(matrix const &lhs, matrix const &rhs);

bool check_precision(float lhs, float rhs, float precision, float *max_diff = nullptr, bool do_show = true);
bool same(qpu::vector const &lhs, qpu::vector const &rhs, float precision = 0.0f);

} // namespace qpu

#endif // _INCLUDE_RNNSUPPORT_MATRIX_H
