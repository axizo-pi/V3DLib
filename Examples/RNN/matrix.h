#ifndef _INCLUDE_RNN_MATRIX
#define _INCLUDE_RNN_MATRIX
#include "Kernel.h"
#include "Kernels.h"
#include "Support/basics.h"
#include "Support/Settings.h"
#include "tools.h"
#include "Source/Float.h"

using namespace Log;
using namespace V3DLib;

template<int const Size>
struct vector;

/**
 * width is in multiples of 16.
 *
 * width x height = 16*16 x 16:
 * - has been shown to work
 * - are dimension which I consider to efficiently use the QPU
 */
template<int const Width, int const Height>
struct matrix {
	using Class = matrix<Width, Height>;

	matrix() : 
		m_width(16*Width),
		m_height(Height)
	{
		if (m_width < 0) {
			cerr << "vector ctor: width must be positive" << thrw;
		}
		if (m_height < 0) {
			cerr << "vector ctor: height must be positive" << thrw;
		}

		m_arr.reset(new Float::Array(m_width*m_height));

		init_static();
	}


	matrix(matrix &rhs) {
		*this = rhs;
		init_static();
	}


	int width() const  { return m_width; }
	int height() const { return m_height; }

	Float::Array &operator &() {
		assert(m_arr != nullptr);
		return (*m_arr);
	}


	Float::Array &arr() {
		assert(m_arr != nullptr);
		return *m_arr;
	}

	Float::Array const &arr() const {
		assert(m_arr != nullptr);
		return *m_arr;
	}

	void rand() {
  	frand_array(arr());
	}


	void set(float init_val) {
		auto &r = arr();

		for (int i = 0; i < m_width*m_height; ++i) {
			r[i] = init_val;
		}
	}


	void frand() {
		assert(arr().size() > 0);

  	for (int i = 0; i < (int) arr().size(); ++i) {
	    arr()[i] = ::frand();
  	}
	}

	void set(Float::Array const &rhs) {
		assert(arr().size() == rhs.size());

		for (int i = 0; i < (int) arr().size(); ++i) {
			arr()[i] = rhs[i];
		}
	}


	Class &operator=(Class &rhs) {
		transfer(rhs);
		return *this;
	}


	std::string dump() const {
		assert(m_arr != nullptr);
		std::string ret;

		for (int h = 0; h < m_height; ++h) {
		 	ret << h << ": " << vector_dump(*m_arr, m_width, h*m_width) << "\n";
		}

		return ret;
	}


	vector<Height/16> operator*(vector<Width> const &rhs) {
		vector<Height/16> ret;

  	m_mult->load(&rhs.arr(), &arr(), &ret.arr()).run();
		return ret;
	}

protected:
	void width(int rhs) { m_width = rhs; }

	void transfer(Class const &rhs) {
		assert(rhs.m_arr != nullptr);

		m_width  = rhs.m_width;
		m_height = rhs.m_height;

		//m_arr.reset(rhs.m_arr.release());
		m_arr = rhs.m_arr;
		//assert(rhs.m_arr == nullptr);
	}

	std::shared_ptr<Float::Array> m_arr;

private:
	int m_width = 0;
	int m_height = 0;

	static BaseKernel *m_mult;

	void init_static() {
		if (m_mult == nullptr) {
			m_mult = new BaseKernel(compile(kernel<Height, Width>, settings()));
		}
	}
};


template<int const Width, int const Height>
BaseKernel *matrix<Width, Height>::m_mult = nullptr;


/**
 * size is in multiples of 16
 */
template<int const Size>
struct vector : public matrix<Size, 1> {
	using Class = vector<Size>;
	using Parent = matrix<Size,1>;

	vector(vector &rhs) : Parent() {
		*this = rhs;
		init_static();
	}
	

	vector() : Parent() {
		init_static();
	}

	//
	// It's unfortunate that these are required
	//
	void set(float rhs) { Parent::set(rhs); }

	
	Float::Array &arr() {
		assert(Parent::m_arr != nullptr);
		return *Parent::m_arr;
	}


	// Would have really appreciated having the deleted out const in
	Float::Array /*const*/ &arr() const {
		assert(Parent::m_arr != nullptr);
		return *Parent::m_arr;
	}

	// End unfortunate

	void set(float *rhs, int in_size) {
		assert(Parent::width() >= in_size);

		auto &r = Parent::arr();

		for (int i = 0; i < in_size; ++i) {
			r[i] = rhs[i];
		}

		for (int i = in_size; i < Parent::width(); ++i) {
			r[i] = 0;
		}
	}


	float &operator[] (int index) {
		assert(Parent::width() > index);

		auto &r = Parent::arr();
		return r[index];
	}


	vector operator-(vector &rhs) {
		assert(Parent::width() == rhs.width());
		assert(Parent::height() == 1);
		assert(Parent::height() == rhs.height());

		vector ret;
		assert(ret.width() == Parent::width());

		m_sub->load(&Parent::arr(), &rhs.arr(), &ret.arr()).run();
		return ret;
	}


	vector &operator=(vector const &rhs) {
		Parent::transfer(rhs);
		return *this;
	}


	std::string dump() const {
	 	return vector_dump(Parent::arr(), Parent::width());
	}


	matrix<Size, 16*Size> outer(vector const &rhs) {
		matrix<Size, 16*Size> ret;

		m_op->load(&Parent::arr(), &rhs.arr(), &ret.arr()).run();

		return ret;	
	}


	vector sigmoid(vector const &bias) {
		vector ret;

		m_sigmoid->load(&Parent::arr(), &bias.arr(), &ret.arr()).run();
		return ret;	
	}


	static BaseKernel &op_kernel() {
		init_static();
		return *m_op;
	}


private:

	// TODO: should probably clean this up
	// For now, it works
	static BaseKernel *m_sub;
	static BaseKernel *m_op;
	static BaseKernel *m_sigmoid;

	static void init_static() {
		if (m_sub == nullptr) {
			m_sub = new BaseKernel(compile_b(vector_sub<Size>, settings()));
		}
		if (m_op == nullptr) {
			m_op = new BaseKernel(compile(outer_product<Size>, settings()));
		}
		if (m_sigmoid == nullptr) {
			m_sigmoid = new BaseKernel(compile(kernel_sigmoid<Size>, settings()));
		}
	}
};


template<int const size> BaseKernel *vector<size>::m_sub     = nullptr;
template<int const size> BaseKernel *vector<size>::m_op      = nullptr;
template<int const size> BaseKernel *vector<size>::m_sigmoid = nullptr;

#endif // _INCLUDE_RNN_MATRIX
