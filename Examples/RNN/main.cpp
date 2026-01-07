/**
 * Support for Reverse Neural Networks.
 *
 * For a RNN, this example is tiny. It is in fact below the granularity of the QPU's.
 *
 * Size (input x hidden x output): (30 x 5 x 3) all bits.
 *
 * The goal is to put the basic calculation in place.
 *
 * Adapted from: https://www.geeksforgeeks.org/numpy/implementation-of-neural-network-from-scratch-using-numpy/
 */
#include "model.h"
#include "test.h"
#include "Support/basics.h"
#include "Source/Functions.h"  // rotate
#include "Support/Timer.h"

/**
 * 3 inputs: A, B and C (if you squint)
 */
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


/**
 * Output of 3 bits, to determine A, B or C
 */
const int label_size = 3;
float labels[3][label_size] = {
	{1, 0, 0},
  {0, 1, 0},
  {0, 0, 1}
};


int main(int argc, const char *argv[]) {
  settings().init(argc, argv);

	model<2> k_model(M);

	//test_outer_product(vector<2>::op_kernel());
	//test_vector();

	vector<N> input[3];
	input[0].set(a[0], 30);

	vector<1> desired[3];
	desired[0].set(labels[0], label_size);

	auto result = k_model.forward(input[0]);

 // warn << "Scalar a2 : " << vector_dump(k_model.s_tmp, 16);
 // warn << "kernel res: " << result.dump();
	warn << "Loss: " << loss(result.arr(), desired[0].arr());

	k_model.back_prop(input[0], desired[0]);

  return 0;
}
