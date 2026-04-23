#include "matrix.h"
#include "scalar.h"
#include "helpers.h"  // frrand()

namespace qpu {
namespace {
	
void frand_array(Float::Array &rhs) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    rhs[i] = frand();
  }
}

} // anon namespace


////////////////////////////////////////////////
// Class matrix
////////////////////////////////////////////////

BaseKernel *matrix::m_mult_vec = nullptr;

matrix::matrix(int rows, int columns) : m_rows(rows), m_columns(columns) {
	if (m_rows <= 0)     { cerr << "vector ctor: rows must be positive"    << thrw; }
	if (m_columns <= 0)  { cerr << "vector ctor: columns must be positive" << thrw; }

	m_arr.reset(new Float::Array(m_columns*m_rows));

	init_static();
}


matrix::matrix(matrix &rhs) {
	*this = rhs;
	init_static();
}


Float::Array &matrix::operator &() {
	assert(m_arr != nullptr);
	return (*m_arr);
}


Float::Array &matrix::arr() {
	assert(m_arr != nullptr);
	return *m_arr;
}


Float::Array const &matrix::arr() const {
	assert(m_arr != nullptr);
	return *m_arr;
}


void matrix::rand() {
	frand_array(arr());
}


void matrix::set(float init_val) {
	auto &r = arr();

	for (int i = 0; i < m_rows*m_columns; ++i) {
		r[i] = init_val;
	}
}


void matrix::set(Float::Array const &rhs) {
	assert(arr().size() == rhs.size());

	for (int i = 0; i < (int) arr().size(); ++i) {
		arr()[i] = rhs[i];
	}
}


void matrix::frand() {
	assert(arr().size() > 0);

 	for (int i = 0; i < (int) arr().size(); ++i) {
    arr()[i] = frrand();
    //arr()[i] = ::frand();
 	}
}


matrix &matrix::operator=(matrix &rhs) {
	transfer(rhs);
	return *this;
}


matrix matrix::operator-(matrix const &rhs) {
	assert(m_columns == rhs.columns() && m_rows == rhs.rows());
	matrix ret(m_rows, m_columns);

	for (int i = 0; i < (int) m_arr->size(); ++i) {
		ret.arr()[i] = arr()[i] - rhs.arr()[i];
	}

	return ret;
}


matrix &matrix::operator-=(matrix const &rhs) {
	assert(m_columns == rhs.columns() && m_rows == rhs.rows());

	for (int i = 0; i < (int) m_arr->size(); ++i) {
		arr()[i] -= rhs.arr()[i];
	}

	return *this;
}


matrix matrix::operator*(float rhs) {
	matrix ret(m_rows, m_columns);

	for (int i = 0; i < (int) m_arr->size(); ++i) {
		ret.arr()[i] = arr()[i]*rhs;
	}

	return ret;
}


matrix matrix::operator*(matrix const &rhs) const {
	//warn << "Called matrix matrix::operator*()";
	assert(m_columns > 0);
	assert(m_rows > 0);

	if (m_columns != rhs.rows()) {
		cerr << "Matrix::operator*() Inner dimension does not match. "
				 << "this(rows, columns): (" << m_rows << ", " << m_columns << "), "
				 << "rhs(rows, columns):  (" << rhs.rows() << ", " << rhs.columns() << ")"
		 		 << thrw;
	}
	//assert(m_columns == rhs.rows());    // Inner dimension must match

	assert((m_columns % 16) == 0);      // Inner dimension must be multiple of 16

	matrix ret(m_rows, 1);

	V3DLib::timers.start("mult load");
 	m_mult_vec->load(&rhs.arr(), &arr(), &ret.arr(), m_columns/16, m_rows);
	V3DLib::timers.stop("mult load");

	V3DLib::timers.start("mult run");
 	m_mult_vec->run();
	V3DLib::timers.stop("mult run");

	return ret;
}


/**
 * NOTE: this is currently a scalar operation
 */
matrix matrix::sigmoid_derivative(matrix const &rhs) {
	assert(m_columns == rhs.columns() && m_rows == rhs.rows());

	matrix ret(m_rows, m_columns);

	for (int i = 0; i < (int) m_arr->size(); ++i) {
		float el = arr()[i];
		ret.arr()[i] = el*(1 - el);
	}

	for (int i = 0; i < (int) ret.arr().size(); ++i) {
		float a = ret.arr()[i];
		float b = rhs.arr()[i];

		ret.arr()[i] = a*b;
	}

	return ret;
}


matrix matrix::transpose() const {
	auto &t_3 = V3DLib::timers.start("matrix transpose");
	matrix ret(m_columns, m_rows);

	for (int h = 0; h < m_rows; ++h) {
		for (int w = 0; w < m_columns; ++w) {
			ret.arr()[w*m_rows + h] = arr()[h*m_columns + w];
		}
	}

	t_3.stop();
	return ret;
}


std::string matrix::dump_dimensions() const {
	std::string ret;
	ret << "(" << m_rows << ", " << m_columns << ") ";
	return ret;
}


std::string matrix::dump(bool output_int) const {
	assert(m_arr != nullptr);
	std::string ret;

	ret << "Here " << dump_dimensions();

	if (m_columns == 1) {
		ret << "(tr) ";
		ret << "[" << vector_dump(*m_arr, m_rows, 0, output_int) << "]";
	} else {
		ret << "\n";

		for (int h = 0; h < m_rows; ++h) {
		 	ret << h << ": [" << vector_dump(*m_arr, m_columns, h*m_columns, output_int) << "]\n";
		}
	}

	return ret;
}


void matrix::transfer(matrix const &rhs) {
	assert(rhs.m_arr != nullptr);

	m_columns  = rhs.m_columns;
	m_rows = rhs.m_rows;

	//m_arr.reset(rhs.m_arr.release());
	m_arr = rhs.m_arr;
	//assert(rhs.m_arr == nullptr);
}


void matrix::init_static() {
	if (m_mult_vec == nullptr) {
		m_mult_vec = new BaseKernel(compile(kernel_mult_vec, settings()));
	}
}


matrix operator*(float scalar, matrix /* const */ &mat) {
	auto ret = mat * scalar;
	return ret;
}


////////////////////////////////////////////////
// Class vector 
////////////////////////////////////////////////

BaseKernel *vector::m_sub     = nullptr;
BaseKernel *vector::m_op      = nullptr;
BaseKernel *vector::m_sigmoid = nullptr;

vector::vector(vector &rhs) : matrix(rhs) {
	*this = rhs;
	init_static();
}


vector::vector(int rows) : matrix(rows, 1) {
	if ((rows & 0xf) != 0) { cerr << "vector ctor: rows must be a multiple of 16" << thrw; }
	init_static();
}


void vector::set(float *rhs, int in_size) {
	assert(rows() >= in_size);

	auto &r = matrix::arr();

	for (int i = 0; i < in_size; ++i) {
		r[i] = rhs[i];
	}

	for (int i = in_size; i < rows(); ++i) {
		r[i] = 0;
	}
}


void vector::set(float init_val) {
	auto &r = arr();

	for (int i = 0; i < size(); ++i) {
		r[i] = init_val;
	}
}


float &vector::operator[](int index) {
	assert(rows() > index);

	auto &r = arr();
	return r[index];
}


float vector::operator[](int index) const {
	assert(rows() > index);

	auto &r = arr();
	return r[index];
}


int vector::size() const {
	assert(columns() == 1);
	return rows();
}


vector vector::operator-(vector const &rhs) {
	assert(columns() == 1);
	assert(columns() == rhs.columns());
	assert(rows() == rhs.rows());
	if ((rows() & 0xf) != 0) { cerr << "vector sub: rows must be a multiple of 16" << thrw; }

	vector ret(rows());

	m_sub->load(&arr(), &rhs.arr(), &ret.arr(), rows()/16).run();
	return ret;
}


vector &vector::operator=(matrix const &rhs) {
	assert(rhs.columns() == 1);
	transfer(rhs);
	return *this;
}


matrix vector::outer(matrix const &rhs) const {
	assert(rhs.columns() == 1);  // Expecting a vector as input
	if ((rows() & 0xf) != 0)     { cerr << "vector outer: expecting rows to be a multiple of 16" << thrw; }
	if ((rhs.rows() & 0xf) != 0) { cerr << "vector outer: expecting rhs rows to be a multiple of 16" << thrw; }

	matrix ret(rows(), rhs.rows());

	m_op->load(&arr(), &rhs.arr(), &ret.arr(), rows(), rhs.rows()/16).run();
	return ret;	
}


vector vector::sigmoid(vector const &bias) {
	vector ret(rows());

	m_sigmoid->load(&arr(), &bias.arr(), &ret.arr(), rows()/16).run();
	return ret;	
}


std::string vector::dump(bool output_int) const {
	std::string ret;
	ret << dump_dimensions()
 	    << vector_dump(arr(), rows(), 0, output_int);

	return ret;
}


BaseKernel &vector::op_kernel() {
	init_static();
	return *m_op;
}


void vector::init_static() {
	if (m_sub     == nullptr) { m_sub     = new BaseKernel(compile_b(vector_sub,   settings())); }
	if (m_op      == nullptr) { m_op      = new BaseKernel(compile(outer_product,  settings())); }
	if (m_sigmoid == nullptr) { m_sigmoid = new BaseKernel(compile(kernel_sigmoid, settings())); }
}

} // namespace qpu

