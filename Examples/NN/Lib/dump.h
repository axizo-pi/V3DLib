#ifndef _INCLUDE_RNNSUPPORT_DUMP_H
#define _INCLUDE_RNNSUPPORT_DUMP_H
#include "matrix.h"
#include "Source/Float.h"
#include <string>

class MatrixAdapter {
public:
  virtual int width() const = 0;
  virtual int height() const = 0;

  // Synonyms for previous
  int rows() const    { return height(); }
  int columns() const { return width(); }

  int size() const   { return width()*height(); }
  bool empty() const { return (size() == 0); }
  bool is_zero() const;
  std::string dump_dim() const;

  virtual float at(int r, int c) const = 0;
};


class FloatArrayAdapter: public MatrixAdapter {
public:
  FloatArrayAdapter(V3DLib::Float::Array const &m, int rows, int cols);

  int height() const override { return m_rows; }
  int width()  const override { return m_cols; }

  float at(int r, int c) const override;

private:  
  V3DLib::Float::Array const &m_m;
  const int m_rows;
  const int m_cols;
};


std::string vector_dump(MatrixAdapter const &src, int start_index, bool output_int);
std::string matrix_dump(MatrixAdapter const &src, bool output_int);
std::string matrix_dump_simple(MatrixAdapter const &src);

#endif // _INCLUDE_RNNSUPPORT_DUMP_H
