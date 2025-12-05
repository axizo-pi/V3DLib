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

const int M = 16;  // Num of rows in matrix
const int N = 16;  // Width of matrix and length of vector

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

  // Generate a number from 0.0 to 1.0, inclusive.
  float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

/*
  // Generate a number from 0.0 to some arbitrary float, X
  const float X = 1.0f;
  float r2 = static_cast <float> (rand()) / (static_cast <float> (((float) RAND_MAX)/X));

  // Generate a number from some arbitrary LO to some arbitrary HI:
  const float LO = 0.0f;
  const float HI = 1.0f;
  float r3 = LO + static_cast <float> (rand()) /( static_cast <float> (((float) RAND_MAX)/(HI-LO)));
*/

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


void kernel_sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out) {
	Float x = *in;

	x += *bias;
	x = (1.0/(1.0 + exp(-1.0*x)));

	*out = x;
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


/**
 * Separate test for outer product
 */
void test_outer_product(BaseKernel &op) {

	float primes[2*16]=
	{ 2,	3,	5,	7,	11,	13,	17,	19,	23,	29,	31,	37,	41,	43,	47,	53,	59,	61,	67,	71,
	 73,	79,	83,	89,	97,	101,	103,	107,	109,	113,	127,	131 //	137	139	149	151	157	163	167	173
	};

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


struct model {
	model(int n_size, int m_size) :
		w1(n_size*m_size),
		bias1(n_size),
		z1(n_size),
		a1(n_size),

	 	w2(n_size*m_size),
		bias2(n_size),
		z2(n_size),
		a2(n_size),

  	k(compile(kernel<M, N>, settings)),
		sigmoid(compile_b(kernel_sigmoid, settings)),
		op(compile(outer_product<2>, settings))
	{
  	frand_array(w1);
  	frand_array(bias1);
  	frand_array(w2);
  	frand_array(bias2);
	}

  Float::Array w1;
  Float::Array bias1;
	Float::Array z1;
	Float::Array a1;

  Float::Array w2;
  Float::Array bias2;
	Float::Array z2;
	Float::Array a2;

	BaseKernel k;
	BaseKernel sigmoid;
	BaseKernel op;

	void forward(Float::Array &input) {
    Timer timer("Matrix mult");

  	k.load(&input, &w1, &z1);
    k.run();
  	//run_scalar(input, k_model.w1, k_model.z1);

		sigmoid.load(&z1, &bias1, &a1);
		sigmoid.run();
		//scalar_sigmoid(k_model.z1, k_model.bias1, s_model.a1);

  	k.load(&a1, &w2, &z2);
    k.run();
  	//run_scalar(k_model.a1, k_model.w2, s_model.z2);

		sigmoid.load(&z2, &bias2, &a2);
		sigmoid.run();
		//scalar_sigmoid(k_model.z2, k_model.bias2, s_model.a2);

    timer.end();

		// TODO adjust
    int flops = 2*M*(16*N + 16*(N - 1));  // matrix num_rows*(num_mults + num_adds);
	  flops    += 2*M*6;                    // sigmoid (1.0/(1.0 + exp(-1.0*x + bias)));

    warn << "MFLOPS: " << ((float) flops)/timer.diff()/1.0e6;
    //warn << "Timer diff: " << timer.diff();
	}
};


int main(int argc, const char *argv[]) {
	const int local_N = M;

  settings.init(argc, argv);

	model s_model(16*N, M);
	model k_model(16*N, M);

//	s_model.x.copyFrom(k_model.x);
	s_model.w1 = k_model.w1;
	s_model.bias1 = k_model.bias1;
	s_model.w2 = k_model.w2;
	s_model.bias2 = k_model.bias2;

  //warn << "scalar w2: " << vector_dump(s_model.w2, local_N);
  //warn << "kernel w2: " << vector_dump(k_model.w2, local_N);

	Float::Array input(16*N);
	init_input(input, a[0], 30);
	Float::Array label(16*N);
	init_input(label, labels[0], 3);

	//test_outer_product(k_model.op);

	k_model.forward(input);

	/*
  warn << "scalar z1: " << vector_dump(s_model.z1, local_N);
  warn << "kernel z1: " << vector_dump(k_model.z1, local_N);
  warn << "Scalar a1: " << vector_dump(s_model.a1, local_N);
  warn << "kernel a1: " << vector_dump(k_model.a1, local_N);
  warn << "scalar z2: " << vector_dump(s_model.z2, local_N);
  warn << "kernel z2: " << vector_dump(k_model.z2, local_N);
	*/
  warn << "Scalar a2: " << vector_dump(s_model.a2, local_N);
  warn << "kernel a2: " << vector_dump(k_model.a2, local_N);

  return 0;
}
