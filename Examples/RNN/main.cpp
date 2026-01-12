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

const int NumInputs = 3;

/**
 * 3 inputs: Letters A, B and C (if you squint)
 */
float a[NumInputs][30] = {
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
float labels[NumInputs][label_size] = {
	{1, 0, 0},
  {0, 1, 0},
  {0, 0, 1}
};


int main(int argc, const char *argv[]) {
  settings().init(argc, argv);

	model k_model(32, M);

	//test_outer_product(vector::op_kernel());
	//test_vector();

	vector input[NumInputs] = {32, 32, 32};
	for (int i = 0; i < NumInputs; ++i) {
		input[i].set(a[i], 30);
	}

	vector desired[NumInputs] = {16, 16, 16};
	for (int i = 0; i < NumInputs; ++i) {
		desired[i].set(labels[i], label_size);
	}

	for (int i = 0; i < NumInputs; ++i) {
		auto result = k_model.forward(input[i]);

		warn << "Loss: " << scalar::loss(result.arr(), desired[i].arr());

		k_model.back_prop(input[i], desired[i]);
	}

  return 0;
}
