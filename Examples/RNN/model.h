#ifndef _INCLUDE_RNN_MODEL
#define _INCLUDE_RNN_MODEL
#include "matrix.h"
#include "scalar.h"

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
struct model {
	model(int n_size, int m_size) :
		s_tmp(16*n_size),
  	k(compile(kernel, settings())),
		sigmoid(compile(kernel_sigmoid, settings()))
	{
		w1_v.frand();
		bias1.frand();
		w2.frand();
		bias2.frand();
	}

	float alpha = 1;

	matrix w1_v{16*N, M};
	vector z1_v{16};
  vector bias1{16};
  vector a1{16};

	matrix w2{16, 16};
  vector bias2{16};
  vector z2_v{16};
  vector a2{16};

	Float::Array s_tmp; // For scalar output

	BaseKernel k;
	BaseKernel sigmoid;

	vector forward(vector const &input_v) {
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

  	z2_v = w2 * a1;

  	k.load(&a1.arr(), &w2.arr(), &s_tmp).run();

  	//run_scalar(a1.arr(), w2.arr(), s_tmp);
  	//warn << "scalar z2: " << vector_dump(s_tmp, 16);
  	//warn << "kernel z2: " << z2_v.dump();

		a2 = z2_v.sigmoid(bias2);
		scalar::sigmoid(z2_v.arr(), bias2.arr(), s_tmp);

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


	void back_prop(vector &input, vector &desired) {
  	warn << "desired: " << desired.dump();
  	//warn << "Pre  a2: " << a2.dump();
		auto la2 = forward(input);  // Same as member a2; expected
  	//warn << "Post la2: " << la2.dump();

		//
		// Output layer to hidden layer
		//
		auto d2     = la2 - desired; 
  	warn << "d2: " << d2.dump();

		auto w2_adj = a1.outer(d2);
  	//warn << "w2_adj:\n" << w2_adj.dump();

		auto w2_tmp = w2 - alpha*w2_adj;
  	//warn << "w2_tmp:\n" << w2_tmp.dump();
	
		bias2 -= alpha * d2;
  	warn << "bias2:\n" << bias2.dump();

		//	
		// Hidden layer adjusting w1
		//
		auto tmp1 = w2 * d2;
  	warn << "tmp1:\n" << tmp1.dump();

		auto d1 = a1.sigmoid_derivative(tmp1);
  	warn << "d1:\n" << d1.dump();

		auto w1_adj = input.outer(d1);  // gradient, outer product
  	warn << "w1_adj:\n" << w1_adj.dump();
	}
};

#endif // _INCLUDE_RNN_MODEL
