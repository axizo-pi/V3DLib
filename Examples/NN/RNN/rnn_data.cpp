#include "rnn_data.h"

/**
 * /file
 *
 * Inputs and outputs for original RNN example.
 */

using namespace std;

namespace rnn {
namespace {

const int InputSize  = 30;
const int label_size = 3;

/**
 * 3 inputs: Letters A, B and C (if you squint)
 */
float a[NumInputs][InputSize] = {
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
float labels[NumInputs][label_size] = {
  {1, 0, 0},
  {0, 1, 0},
  {0, 0, 1}
};

}  // anon namespace


Inputs inputs() {
	Inputs ret;

	ret.resize(NumInputs);
	for (int i = 0; i < NumInputs; ++i) {
		ret[i].resize(InputSize);
	  for (int j = 0; j < InputSize; ++j) {
		  ret[i][j] = a[i][j];
		}
	}

	return ret;
}


Inputs expected() {
	Inputs ret;

	ret.resize(NumInputs);
	for (int i = 0; i < NumInputs; ++i) {
		ret[i].resize(label_size);
	  for (int j = 0; j < label_size; ++j) {
		  ret[i][j] = labels[i][j];
		}
	}

	return ret;
}


}  // namespace rnn
