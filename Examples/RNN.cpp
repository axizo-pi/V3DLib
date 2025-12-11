/**
 * Support for Reveirse Neural Networks.
 *
 * Adapted from: https://www.geeksforgeeks.org/numpy/implementation-of-neural-network-from-scratch-using-numpy/
 */
#include "Support/Settings.h"
#include "Source/Float.h"
#include "Support/basics.h"
#include "Kernel.h"
#include "Source/Functions.h"  // rotate
#include "Support/Timer.h"
#include <cmath>

using namespace V3DLib;
using namespace Log;

Settings settings;

float a[3][30] = {
{	
	0, 0, 1, 1, 0, 0,
 	0, 1, 0, 0, 1, 0,
 	1, 1, 1, 1, 1, 1,
 	1, 0, 0, 0, 0, 1,
 	1, 0, 0, 0, 0, 1,
}, {
			0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 1, 0,
   		0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 1, 0,
   		0, 1, 1, 1, 1, 0
}, {				
			0, 1, 1, 1, 1, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 0, 0, 0, 0,
   		0, 1, 1, 1, 1, 0
}};

float labels[3][3] = {
			{1, 0, 0},
   		{0, 1, 0},
   		{0, 0, 1}
};

/**
 * Source: https://stackoverflow.com/questions/686353/random-float-number-generation
 */
float frand() {
  static bool did_init = false;
  if (!did_init) {
    srand (static_cast <unsigned> (time(0)));
    did_init = true;
  }
/*
  // Generate a number from 0.0 to 1.0, inclusive.
  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

  // Generate a number from 0.0 to some arbitrary float, X
  const float X = 1.0f;
  float r = static_cast <float> (rand()) / (static_cast <float> (((float) RAND_MAX)/X));
*/

  // Generate a number from some arbitrary LO to some arbitrary HI:
  const float LO = -1.0f;
  const float HI =  1.0f;
  float r = LO + static_cast <float> (rand()) /( static_cast <float> (((float) RAND_MAX)/(HI-LO)));

  return r;
}


void frand_array(Float::Array &rhs) {
  for (int i = 0; i < (int) rhs.size(); ++i) {
    rhs[i] = frand();
  }
}


/**
 * activation function
 */
float sigmoid(float x) {
	return (float) (1.0/(1.0 + std::exp(-x)));
}


void scalar_sigmoid(Float::Array &vec, Float::Array const &bias, Float::Array &output) {
  for (int h = 0; h < (int) vec.size(); ++h) {
		output[h] = sigmoid(vec[h] + bias[h]); 
  }
}



/**
 * matrix width is assumed to be the same as the vector length
 */
void run_scalar(Float::Array const &vec, Float::Array const &mat, Float::Array &res) {
  int height = mat.size()/vec.size();
  assert(height*vec.size() == mat.size());

  for (int h = 0; h < height; ++h) {
    res[h] = 0;

    int offset = h*vec.size();
    for (int w = 0; w < (int) vec.size(); ++w) {
      res[h] += mat[offset + w] * vec[w];
    }
  }
}


template<int const N>
void kernel_sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out) {

  for (int h = 0; h < N; ++h) {
		Float x = *in;

		x += *bias;
		x = (1.0/(1.0 + exp_e(-1.0*x)));

		*out = x;

		in.inc();
		bias.inc();
		out.inc();
	}
}


template<int const M, int const N>
void kernel(Float::Ptr input, Float::Ptr mat, Float::Ptr result) {
  assert(N > 0);
  assert(M > 0);

  Float tmp[M];
  Float::Ptr row[M];
  Int offset2 = 4*16*N;

  for (int h = 0; h < M; ++h) {
    tmp[h] = 0;
    row[h] = mat.offset(h*offset2);
  }

  for (int i = 0; i < N; ++i) {
    Float v = *input;
  
    for (int h = 0; h < M; ++h) {
      tmp[h] += (*row[h])*v;
    }

    if (i < (N - 1)) {
      for (int h = 0; h < M; ++h) {
        row[h].inc();
      }

      input.inc();
    }
  }

  Float res = 0;

  for (int h = 0; h < M; ++h) {
    rotate_sum(tmp[h], tmp[h]);
    res.set_at(h, tmp[h]);
  }

  *result = res;
}


template<int const N> // length of input vectors, in blocks of 16
void outer_product(Float::Ptr left, Float::Ptr right, Float::Ptr out_matrix) {
	left -= index();
	//Float right_out = toFloat(2*(1 + index()));

  for (int i = 0; i < 16*N; ++i) {
		Float::Ptr start = right;

  	for (int j = 0; j < N; ++j) {
			*out_matrix = *left * *start;
			out_matrix.inc(); start.inc();
		}

		++left;
	}
}


template<int const N> // length of input vectors, in blocks of 16
void vector_sub(Float::Ptr left, Float::Ptr right, Float::Ptr out) {
	Float tmp;

  for (int i = 0; i < N; ++i) {
		tmp = *left - *right; 
		*out = tmp;

		left.inc();
		right.inc();
		out.inc();
	}
}


std::string vector_dump(Float::Array const &src, int size, int start_index = 0) {
	assert(size <= (int) src.size());
	std::string buf;

  for (int h = 0; h < size; ++h) {
    buf << src[start_index + h] << ", ";
  }

	return buf;
}


void init_input(Float::Array &input, float *a,  int n) {
  for (int h = 0; h < n; ++h) {
		input[h] = a[h];
	}
}

namespace {
	// Values for testing purposes

	const int primes_size = 2*16;

	float primes[primes_size]=
	{ 2,	3,	5,	7,	11,	13,	17,	19,	23,	29,	31,	37,	41,	43,	47,	53,	59,	61,	67,	71,
	 73,	79,	83,	89,	97,	101,	103,	107,	109,	113,	127,	131 //	137	139	149	151	157	163	167	173
	};

} // anon namespace


/**
 * Separate test for outer product
 */
void test_outer_product(BaseKernel &op) {
	Float::Array left_outer(2*16); 
	init_input(left_outer, primes, 2*16);

	Float::Array right_outer(2*16); 
	Float::Array result_outer((2*16)*(2*16)); 

	for (int i = 0; i < 2*16; ++i) {
	  right_outer[i] = (float) (i + 1);
	}

	op.load(&left_outer, &right_outer, &result_outer);
	op.run();

	std::string buf;
	buf << "outer product:\n";
	for (int i = 0; i < 2*16; ++i) {
	  buf << i << ": " << vector_dump(result_outer, 2*16, 2*16*i) << "\n";
	}
	warn << buf;
}


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
			m_mult = new BaseKernel(compile(kernel<Height, Width>, settings));
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
			m_sub = new BaseKernel(compile_b(vector_sub<Size>, settings));
		}
		if (m_op == nullptr) {
			m_op = new BaseKernel(compile(outer_product<Size>, settings));
		}
		if (m_sigmoid == nullptr) {
			m_sigmoid = new BaseKernel(compile(kernel_sigmoid<Size>, settings));
		}
	}
};


template<int const size> BaseKernel *vector<size>::m_sub     = nullptr;
template<int const size> BaseKernel *vector<size>::m_op      = nullptr;
template<int const size> BaseKernel *vector<size>::m_sigmoid = nullptr;


void test_vector() {
/*	
	vector<2> a_vec; a_vec.set(3);
	vector<2> b_vec; b_vec.set(-7);
	auto c_vec = a_vec - b_vec;
 	warn << "a_vec: " << a_vec.dump();
 	warn << "sub: " << c_vec.dump();
*/	
	vector<2> d_vec; d_vec.set(primes, primes_size);
	//warn << "d_vec: " << d_vec.dump();

	vector<2> e_vec;

	for (int i = 0; i < 2*16; ++i) {
	  e_vec[i] = (float) (i + 1);
	}

	warn << "e_vec: " << e_vec.dump();

	auto result = d_vec.outer(e_vec);

	warn << "result: " << result.dump();
}

////////////////////////////////////////////////////

const int M = 16; // Num of rows in matrix
const int N = 2;  // Width of matrix and length of vector

/**
 * The layer sizes of the example models are:
 *
 * input : hidden : output = 30 : 5 : 3
 *
 * The kernel(s) however work with array sizes of multiples
 * of 16. We should the smallest 16-item increments that
 * encompass the original size.
 */
template<int const n_size>
struct model {
	model(int m_size) :
		s_tmp(16*n_size),

  	k(compile(kernel<M, n_size>, settings)),
		sigmoid(compile(kernel_sigmoid<n_size>, settings))
	{
		w1_v.frand();
		bias1.frand();
		w2_v.frand();
		bias2.frand();
	}

	matrix<N, M> w1_v;
	vector<1> z1_v;
  vector<1> bias1;
  vector<1> a1;

  //Float::Array w2;
	matrix<1, 16> w2_v;
  vector<1> bias2;
  vector<1> z2_v;
  vector<1> a2;

	Float::Array s_tmp; // For scalar output

	BaseKernel k;
	BaseKernel sigmoid;

	vector<1> forward(vector<2> const &input_v) {
    //Timer timer("Matrix mult");

  	z1_v = w1_v * input_v;
 		//warn << "z1_v: " << z1_v.dump();

  	//run_scalar(input_v.arr(), w1_v.arr(), s_tmp);
  	//warn << "scalar z1: " << vector_dump(s_tmp, 16);
  	//warn << "kernel z1: " << z1_v.dump();

  	//warn << "bias1: " << bias1.dump();

		a1 = z1_v.sigmoid(bias1);

		//scalar_sigmoid(z1_v.arr(), bias1.arr(), s_tmp);
  	//warn << "scalar sigmoid: " << vector_dump(s_tmp, 16);
  	//warn << "kernel sigmoid: " << a1.dump();

  	z2_v = w2_v * a1;

  	k.load(&a1.arr(), &w2_v.arr(), &s_tmp).run();

  	//run_scalar(a1.arr(), w2_v.arr(), s_tmp);
  	//warn << "scalar z2: " << vector_dump(s_tmp, 16);
  	//warn << "kernel z2: " << z2_v.dump();

		a2 = z2_v.sigmoid(bias2);
		scalar_sigmoid(z2_v.arr(), bias2.arr(), s_tmp);

/*
		// TODO adjust
    timer.end();

    int flops = 2*M*(16*N + 16*(N - 1));  // matrix num_rows*(num_mults + num_adds);
	  flops    += 2*M*6;                    // sigmoid (1.0/(1.0 + exp(-1.0*x + bias)));

    //warn << "MFLOPS: " << ((float) flops)/timer.diff()/1.0e6;
    //warn << "Timer diff: " << timer.diff();
*/		

		return a2;
	}

	void back_prop(vector<2> &input, vector<1> &result) {
  	warn << "Pre  a2: " << a2.dump();
		auto la2 = forward(input);  // This does not change the value of a2!
  	warn << "Post a2: " << la2.dump();

		auto d2 = la2 - result; 
	}
};


int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

	model<2> k_model(M);

	//test_outer_product(vector<2>::op_kernel());
	//test_vector();

	vector<N> input;
	input.set(a[0], 30);


	auto result = k_model.forward(input);

 // warn << "Scalar a2 : " << vector_dump(k_model.s_tmp, 16);
 // warn << "kernel res: " << result.dump();

	k_model.back_prop(input, result);

  return 0;
}
