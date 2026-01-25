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
const int NumEpochs = 1000;

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



void train(vector const *inputs, vector const *desired, model &k_model) {
	int   epoch = 0;
	float loss[NumInputs];

	int epoch_step = 1;
	if (NumEpochs >= 1000) {
		epoch_step = 20;
	} else if (NumEpochs >= 400) {
		epoch_step = 10;
	}

	while (epoch < NumEpochs) {
		for (int i = 0; i < NumInputs; ++i) {
			auto result = k_model.forward(inputs[i]);

			loss[i] = scalar::loss(result.arr(), desired[i].arr());
			k_model.back_prop(inputs[i], desired[i]);
		}

		float sum = 0;
		for (int i = 0; i < NumInputs; ++i) {
			sum += loss[i];
		}

		sum /= ((float) NumInputs);  // Take average

		if (epoch % epoch_step == 0) {
			warn << "epoch: " << epoch << " ======== acc: " << (1 - sum)*100;
		}
		epoch++;
	}
}


void predict(vector const &input, model &k_model) {
	auto result = k_model.forward(input);
	warn << "predict result: " << result.dump();

	float maxm = 0;
	int k = 0;

	for (int i = 0; i < NumInputs; ++i) {
		float n = result[i];

		if (maxm < n) {
			maxm = n;
			k = i;
		}
	}

	switch (k) {
		case 0: warn << "Image is of letter A."; break;
		case 1: warn << "Image is of letter B."; break;
		case 2: warn << "Image is of letter C."; break;

		default: assert(false); break;
	}
}


int main(int argc, const char *argv[]) {
  settings().init(argc, argv);

	if (false) {
		test_back_propagation(vector::op_kernel());
		//test_outer_product(vector::op_kernel(), true);
		//test_vector();
		return 0;
	}

	vector inputs[NumInputs] = {32, 32, 32};
	for (int i = 0; i < NumInputs; ++i) {
		inputs[i].set(a[i], 30);
	}

	vector desired[NumInputs] = {16, 16, 16};
	for (int i = 0; i < NumInputs; ++i) {
		desired[i].set(labels[i], label_size);
	}

	// Using this as a global var leads to segmentation fault
	model k_model(N*16, M);

	train(inputs, desired, k_model);

	predict(inputs[0], k_model);
	predict(inputs[1], k_model);
	predict(inputs[2], k_model);

  return 0;
}
