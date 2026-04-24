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

	matrix(int rows, int columns);
	matrix(matrix &rhs);

	int columns() const  { return m_columns; }
	int rows() const     { return m_rows; }

	Float::Array &operator &();
	Float::Array &arr();
	Float::Array const &arr() const;
	void rand();
	void set(float init_val);
	void set(Float::Array const &rhs);
	void frand();

	matrix &operator=(matrix &rhs);
	matrix operator-(matrix const &rhs);
	matrix &operator-=(matrix const &rhs);
	matrix operator*(float rhs);
	matrix operator*(matrix const &rhs) const;
	matrix sigmoid_derivative(matrix const &rhs);
	matrix transpose() const;

	std::string dump(bool output_int = false) const;
	float &at(int i, int j);
	float at(int i, int j) const;

protected:
	void rows(int rhs)    { m_rows = rhs; }
	void columns(int rhs) { m_columns = rhs; }

	void transfer(matrix const &rhs);
	std::string dump_dim() const;

	std::shared_ptr<Float::Array> m_arr;

private:
	int m_rows     = 0;
	int m_columns  = 0;

	static BaseKernel *m_mult_vec;

	void init_static();
};


matrix operator*(float scalar, matrix /* const */ &mat);


////////////////////////////////////////////////
// Class vector 
////////////////////////////////////////////////

/**
 * Size must be a multiple of 16
 */
struct vector : public matrix {
	vector(vector &rhs);
	vector(matrix rhs);
	vector(int rows);

	void set(float *rhs, int in_size);
	void set(float init_val);
	float &operator[](int index);
	float operator[](int index) const;
	int size() const;

	vector operator-(vector const &rhs);
	vector operator+(vector const &rhs);
	vector mul(vector const &rhs);
	vector &operator=(matrix const &rhs);
	matrix outer(matrix const &rhs) const;
	vector sigmoid(vector const &bias);
	vector tanh();
	static BaseKernel &op_kernel();

	std::string dump(bool output_int = false) const;

private:

	// TODO: should probably clean this up.
	//       ptr's not cleaned up on exit, better would be shared or unique ptr.
	// For now, it works
	static BaseKernel *m_sub;
	static BaseKernel *m_add;
	static BaseKernel *m_op;
	static BaseKernel *m_sigmoid;
	static BaseKernel *m_tanh;

	static void init_static();
};

} // namespace qpu

#endif // _INCLUDE_RNNSUPPORT_MATRIX_H
