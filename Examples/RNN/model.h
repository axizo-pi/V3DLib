#ifndef _INCLUDE_RNN_MODEL
#define _INCLUDE_RNN_MODEL
#include "matrix.h"
#include "scalar.h"
//#include "Source/Float.h"

using namespace V3DLib;

const int M = 16; // Num of rows in matrix
const int N = 2;  // Width of matrix and length of vector

/**
 * The layer sizes of the example models are:
 *
 * input : hidden : output = 30 : 5 : 3
 *
 * The kernel(s) however work with array sizes of multiples
 * of 16. We should use the smallest 16-item increments that
 * encompass the original size.
 */
struct model {
	model(int n_size, int m_size) : s_tmp(n_size) {
		w1.frand();
		bias1.frand();
		w2.frand();
		bias2.frand();
	}

	float alpha = 1;

	matrix w1{16*N, M};
	vector z1{16};
  vector bias1{16};
  vector a1{16};

	matrix w2{16, 16};
  vector bias2{16};
  vector z2{16};
  vector a2{16};

	Float::Array s_tmp{16}; // For scalar output

	vector forward(vector const &input_v) {
    //Timer timer("Matrix mult");

 		//warn << "input_v: " << input_v.dump();
 		//warn << "w1:\n"     << w1.dump();

  	z1 = w1 * input_v;
		//scalar::mult(input_v.arr(), w1.arr(), s_tmp);
 		//warn << "z1       : " << z1.dump();
  	//warn << "scalar z1: " << vector_dump(s_tmp, 16);

  	//warn << "bias1: " << bias1.dump();
		a1 = z1.sigmoid(bias1);
		//scalar::sigmoid(z1.arr(), bias1.arr(), s_tmp);
  	//warn << "kernel sigmoid: " << a1.dump();
  	//warn << "scalar sigmoid: " << vector_dump(s_tmp, 16);

  	z2 = w2 * a1;
		//scalar::mult(a1.arr(), w2.arr(), s_tmp);
  	//warn << "kernel z2: " << z2.dump();
  	//warn << "scalar z2: " << vector_dump(s_tmp, 16);

		a2 = z2.sigmoid(bias2);
		//scalar::sigmoid(z2.arr(), bias2.arr(), s_tmp);
  	//warn << "kernel sigmoid: " << a2.dump();
  	//warn << "scalar sigmoid: " << vector_dump(s_tmp, 16);

/*
		// TODO generalize FLOP calculations for all operations.
		//      Stuff below is embryonic.
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
		auto d2     = la2 - desired;            // error in output layer
  	//warn << "d2: " << d2.dump();          // TODO: only top 3 elements of error (d2) are significant,
 		                                        //       consider setiting rest to 0.

		auto w2_adj = a1.outer(d2);             // gradient, outer product
  	//warn << "w2_adj:\n" << w2_adj.dump();

		auto w2_tmp = w2 - alpha*w2_adj;
  	//warn << "w2_tmp:\n" << w2_tmp.dump();
	
		bias2 -= alpha * d2;
  	//warn << "bias2: " << bias2.dump();

		//	
		// Hidden layer adjusting w1
		//
		auto tmp1 = w2 * d2;
  	//warn << "tmp1: " << tmp1.dump();

		auto d1 = a1.sigmoid_derivative(tmp1);
  	warn << "d1: " << d1.dump();

		auto w1_trans = input.outer(d1);        // gradient, outer product; should be transposed
  	//warn << "w1_trans:\n" << w1_trans.dump();
		auto w1_adj = w1_trans.transpose();
  	//warn << "w1_adj:\n" << w1_adj.dump();

		w1 -= alpha*w1_adj;
  	//warn << "w1 post:\n" << w1.dump();

  	//warn << "bias1 pre :" << bias1.dump();
		bias1 -= alpha * d1;
  	//warn << "bias1 post:" << bias1.dump();

		w2 = w2_tmp;
	}
};

#endif // _INCLUDE_RNN_MODEL
