#ifndef _INCLUDE_RNN_MODEL
#define _INCLUDE_RNN_MODEL
#include "matrix.h"
#include "Static.h"

using namespace V3DLib;

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

  	k(compile(kernel<M, n_size>, settings())),
		sigmoid(compile(kernel_sigmoid<n_size>, settings()))
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

#endif // _INCLUDE_RNN_MODEL
