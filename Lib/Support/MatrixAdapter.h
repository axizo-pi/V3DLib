#ifndef _V3DLIB_SUPPORT_MATRIXADAPTER_H
#define _V3DLIB_SUPPORT_MATRIXADAPTER_H
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


std::string vector_dump(MatrixAdapter const &src, int start_index, bool output_int, bool transpose = false);
std::string matrix_dump(MatrixAdapter const &src, bool output_int);
std::string matrix_dump_simple(MatrixAdapter const &src);

#endif  // _V3DLIB_SUPPORT_MATRIXADAPTER_H
