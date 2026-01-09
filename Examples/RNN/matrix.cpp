#include "matrix.h"
#include "scalar.h"


////////////////////////////////////////////////
// Class matrix
////////////////////////////////////////////////

BaseKernel *matrix::m_mult = nullptr;


matrix::matrix(int width, int height) : m_width(width), m_height(height) {
	if (m_width <= 0)        { cerr << "vector ctor: width must be positive" << thrw; }
	if ((m_width & 16) != 0) { cerr << "vector ctor: width must be a multiple of 16" << thrw; }
	if (m_height <= 0)       { cerr << "vector ctor: height must be positive" << thrw; }

	m_arr.reset(new Float::Array(m_width*m_height));

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
	scalar::frand_array(arr());
}


void matrix::set(float init_val) {
	auto &r = arr();

	for (int i = 0; i < m_width*m_height; ++i) {
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
    arr()[i] = ::frand();
 	}
}


matrix &matrix::operator=(matrix &rhs) {
	assert(m_width == rhs.width() && m_height == rhs.height());
	transfer(rhs);
	return *this;
}


matrix matrix::operator-(matrix const &rhs) {
	assert(m_width == rhs.width() && m_height == rhs.height());
	matrix ret(m_width, m_height);

	for (int i = 0; i < (int) m_arr->size(); ++i) {
		ret.arr()[i] = arr()[i] - rhs.arr()[i];
	}

	return ret;
}


matrix &matrix::operator-=(matrix const &rhs) {
	assert(m_width == rhs.width() && m_height == rhs.height());

	for (int i = 0; i < (int) m_arr->size(); ++i) {
		arr()[i] -= rhs.arr()[i];
	}

	return *this;
}


matrix matrix::operator*(float rhs) {
	matrix ret(m_width, m_height);

	for (int i = 0; i < (int) m_arr->size(); ++i) {
		ret.arr()[i] = arr()[i]*rhs;
	}

	return ret;
}


matrix matrix::operator*(matrix const &rhs) {
	assert(m_width == rhs.height());  // Inner dimension must match
	matrix ret(1,m_height);

 	m_mult->load(&rhs.arr(), &arr(), &ret.arr()).run();
	return ret;
}


/**
 * NOTE: this is currently a scalar operation
 */
matrix matrix::sigmoid_derivative(matrix const &rhs) {
	assert(m_width == rhs.width() && m_height == rhs.height());

	matrix ret(m_width, m_height);

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


std::string matrix::dump() const {
	assert(m_arr != nullptr);
	std::string ret;

	for (int h = 0; h < m_height; ++h) {
	 	ret << h << ": " << vector_dump(*m_arr, m_width, h*m_width) << "\n";
	}

	return ret;
}


void matrix::transfer(matrix const &rhs) {
	assert(rhs.m_arr != nullptr);

	m_width  = rhs.m_width;
	m_height = rhs.m_height;

	//m_arr.reset(rhs.m_arr.release());
	m_arr = rhs.m_arr;
	//assert(rhs.m_arr == nullptr);
}


void matrix::init_static() {
	if (m_mult == nullptr) {
		m_mult = new BaseKernel(compile(kernel, settings()));
	}
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


vector::vector(int height) : matrix(1, height) {
	init_static();
}


void vector::set(float *rhs, int in_size) {
	assert(width() >= in_size);

	auto &r = matrix::arr();

	for (int i = 0; i < in_size; ++i) {
		r[i] = rhs[i];
	}

	for (int i = in_size; i < width(); ++i) {
		r[i] = 0;
	}
}


float &vector::operator[] (int index) {
	assert(width() > index);

	auto &r = arr();
	return r[index];
}


vector vector::operator-(vector &rhs) {
	assert(width() == 1);
	assert(width() == rhs.width());
	assert(height() == rhs.height());

	vector ret(height());

	m_sub->load(&arr(), &rhs.arr(), &ret.arr()).run();
	return ret;
}


vector &vector::operator=(matrix const &rhs) {
	assert(rhs.width() == 1);
	transfer(rhs);
	return *this;
}


matrix vector::outer(matrix const &rhs) {
	assert(rhs.width() == 1);
	matrix ret(width(), rhs.height());

	m_op->load(&arr(), &rhs.arr(), &ret.arr()).run();
	return ret;	
}


vector vector::sigmoid(vector const &bias) {
	vector ret(height());

	m_sigmoid->load(&arr(), &bias.arr(), &ret.arr()).run();
	return ret;	
}


std::string vector::dump() const {
 	return vector_dump(arr(), width());
}


BaseKernel &vector::op_kernel() {
	init_static();
	return *m_op;
}


void vector::init_static() {
	if (m_sub == nullptr)     { m_sub     = new BaseKernel(compile_b(vector_sub,   settings())); }
	if (m_op == nullptr)      { m_op      = new BaseKernel(compile(outer_product,  settings())); }
	if (m_sigmoid == nullptr) { m_sigmoid = new BaseKernel(compile(kernel_sigmoid, settings())); }
}
