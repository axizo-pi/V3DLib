#include "model.h"
#include "test.h"
#include "Support/basics.h"
#include "Source/Functions.h"  // rotate
#include "helpers.h"
#include "global/Timer.h"
#include "rnn_data.h"

/**
 * /file
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

const int NumEpochs = 1000;


void train(vector const *inputs, vector const *desired, model &k_model) {
  int   epoch = 0;
  float loss[rnn::NumInputs];

  int epoch_step = 1;
  if (NumEpochs >= 1000) {
    epoch_step = 50;
  } else if (NumEpochs >= 400) {
    epoch_step = 10;
  }

  while (epoch < NumEpochs) {
    for (int i = 0; i < rnn::NumInputs; ++i) {
      auto result = k_model.forward(inputs[i]);

      loss[i] = scalar::loss(result.arr(), desired[i].arr());

      k_model.back_prop(inputs[i], desired[i]);
    }

    float sum = 0;
    for (int i = 0; i < rnn::NumInputs; ++i) {
      sum += loss[i];
    }

    sum /= ((float) rnn::NumInputs);  // Take average

    if (epoch % epoch_step == 0) {
      warn << "epoch: " << epoch << " ======== acc: " << fixed << (1 - sum)*100;
    }
    epoch++;
  }
}


void predict(vector const &input, model &k_model) {
  auto result = k_model.forward(input);
  //warn << "predict result: " << result.dump();
  //warn << "Till Here" << thrw;

  float maxm = 0;
  int k = 0;

  for (int i = 0; i < rnn::NumInputs; ++i) {
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


/**
 * @brief Perform internal tests
 */
void do_tests() {
  // To compare pseudo-random values with ruby   
  if (false) {
    std::cout.precision(6);

    std::string buf;
    for (int x = 0; x < 1000; ++x) {
      buf << frrand() << ", ";

      if (x % 32 == 0) {
        buf << "\n";
      }
    }

    std::cout << buf << "\n";
    return;
  }

  if (false) {
    test_back_propagation(vector::op_kernel());
    //test_outer_product(vector::op_kernel(), true);
    //test_vector();
    return;
  }
}


int main(int argc, const char *argv[]) {
  settings().init(argc, argv);

  //do_tests(); return 0;

  auto rnn_inputs    = rnn::inputs();
  const int rnn_size = (int) rnn_inputs.size();

  vector inputs[rnn_size] = {32, 32, 32};
  for (int i = 0; i < rnn_size; ++i) {
    inputs[i].set(rnn_inputs[i]);
  }

  auto rnn_expected   = rnn::expected();
  const int rnn_size2 = (int) rnn_expected.size();

  vector desired[rnn_size2] = {16, 16, 16};
  for (int i = 0; i < rnn_size2; ++i) {
    desired[i].set(rnn_expected[i]);
  }

  // Using this as a global var leads to segmentation fault
  model k_model(N*16, M);

  train(inputs, desired, k_model);

  predict(inputs[0], k_model);
  predict(inputs[1], k_model);
  predict(inputs[2], k_model);

  timers.end();
  return 0;
}
