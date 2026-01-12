#ifndef _INCLUDE_RNN_MATRIX
#define _INCLUDE_RNN_MATRIX
#include "kernels.h"
#include "Support/basics.h"
#include "Support/Settings.h"
#include "tools.h"

using namespace Log;
using namespace V3DLib;


////////////////////////////////////////////////
// Class matrix
////////////////////////////////////////////////

/**
 * Width must be in multiples of 16.
 *
 * Case width x height = 16*16 x 16:
 * - has been shown to work
 * - are dimension which I consider to efficiently use the QPU
 */
struct matrix {

	matrix(int width, int height);
	matrix(matrix &rhs);

	int width() const  { return m_width; }
	int height() const { return m_height; }

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
	matrix operator*(matrix const &rhs);
	matrix sigmoid_derivative(matrix const &rhs);
	matrix transpose() const;

	std::string dump() const;

protected:
	void width(int rhs) { m_width = rhs; }

	void transfer(matrix const &rhs);

	std::shared_ptr<Float::Array> m_arr;

private:
	int m_width  = 0;
	int m_height = 0;

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
	vector(int height);
	
/*
	vector() : matrix() {
		init_static();
	}
*/

	//
	// It's unfortunate that these are required
	// TODO: check again
	//
	void set(float rhs) { matrix::set(rhs); }

/*	
	Float::Array &arr() {
		assert(m_arr != nullptr);
		return *m_arr;
	}


	// Would have really appreciated having the deleted out const in
	// TODO: check again
	Float::Array / *const * / &arr() const {
		assert(m_arr != nullptr);
		return *m_arr;
	}
*/
	// End unfortunate

	void set(float *rhs, int in_size);
	float &operator[] (int index);

	vector operator-(vector &rhs);
	vector &operator=(matrix const &rhs);
	matrix outer(matrix const &rhs);
	vector sigmoid(vector const &bias);
	static BaseKernel &op_kernel();

	std::string dump() const;

private:

	// TODO: should probably clean this up
	// For now, it works
	static BaseKernel *m_sub;
	static BaseKernel *m_op;
	static BaseKernel *m_sigmoid;

	static void init_static();
};



#endif // _INCLUDE_RNN_MATRIX
