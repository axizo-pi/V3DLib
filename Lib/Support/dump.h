#ifndef _INCLUDE_RNNSUPPORT_DUMP_H
#define _INCLUDE_RNNSUPPORT_DUMP_H
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


struct CompareStats {
  CompareStats(bool fail_on_first = false) : m_fail_on_first(fail_on_first) { reset(); }

  void reset();
  bool failed() const;
  bool fail_on_first() const { return m_fail_on_first; }
  std::string dump(bool show_first_fail = false) const;

  int   first_i;
  int   first_j;
  int   total;
  int   exact;
  int   same;
  int   zeroes;
  float max_diff;
  int   max_bit;

private:
  const bool m_fail_on_first;
};  


std::string vector_dump(MatrixAdapter const &src, int start_index, bool output_int, bool transpose = false);
std::string matrix_dump(MatrixAdapter const &src, bool output_int);
std::string matrix_dump_simple(MatrixAdapter const &src);

bool check_precision(
  float lhs,
  float rhs,
  int bit_diff = -1,
  CompareStats *stats = nullptr,
  bool do_show = true
);


bool same_intern(
  MatrixAdapter const &lhs,
  MatrixAdapter const &rhs,
  int bit_diff,
  CompareStats &stats,
  bool do_show = true
);

bool same(MatrixAdapter const &lhs, MatrixAdapter const &rhs, int bit_diff = -1, bool show_stats = false); 

namespace bitdiff_stats {

void add(float lhs, float rhs, int id);
void add(MatrixAdapter const &lhs, MatrixAdapter const &rhs, int id);
void dump();

} // namespace  bitdiff_stats

#endif // _INCLUDE_RNNSUPPORT_DUMP_H
