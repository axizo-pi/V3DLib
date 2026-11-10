#include <network.h>     // This and next from nnetcpp
#include <dense.h>
#include <activation.h>
#include <utils.h>       // from nnetcpp tests
#include "rnn_data.h"    // NOTE: this and cpp file copied from RNN
#include "global/Log.h"  // this and next from V3DLib
#include "global/debug.h"
#include <string>        // globals
#include <iostream>

namespace {

std::string dump(Vector const &vec) {
	std::stringstream buf;

	buf << "(rows, cols): (" << vec.rows() << ", " << vec.cols() << ") transposed: "
		  << "<" << vec.transpose() << ">";

	return buf.str();
}


std::string dump(std::vector<Vector> const &input) {
	assert(!input.empty());
	std::stringstream buf;
	auto const &vec = input[0];

	buf << "Size: " << (int) input.size() << ", "
		  << "(rows, cols): (" << vec.rows() << ", " << vec.cols() << ")\n"
	    << "Items transposed:";

	for (int i = 0; i < (int) input.size(); ++i) {
	  auto const &cur = input[i];
		assert(cur.rows() == vec.rows() && cur.cols() == vec.cols());

		buf << "\n  " << i << ": <" << cur.transpose() << ">";
	}

	return buf.str();
}


/**
 * Taken from README of project `nnetcpp`
 */	
Network *make_perceptron() {
	// TODO: Should be made more generic; fine for current test
	const int num_inputs  = 30;
	const int num_hidden  = 5;
	const int num_outputs = 3;

	Network *net = new Network(num_inputs);

	Dense *layer1 = new Dense(num_hidden, 0.005);
	SigmoidActivation *layer1_act = new SigmoidActivation;
	Dense *layer2 = new Dense(num_outputs, 0.005);
	SigmoidActivation *layer2_act = new SigmoidActivation;

	layer1->setInput(net->inputPort());
  layer1_act->setInput(layer1->output());
  layer2->setInput(layer1_act->output());
  layer2_act->setInput(layer2->output());

	net->addNode(layer1);
  net->addNode(layer1_act);
  net->addNode(layer2);
  net->addNode(layer2_act);

	return net;
}


void load_data(std::vector<Vector> &input, std::vector<Vector> &output) {
	auto rnn_inputs    = rnn::inputs();
	auto rnn_expected  = rnn::expected();
	assert(rnn_inputs.size() == rnn_expected.size());

	input.resize(rnn_inputs.size());
	output.resize(rnn_expected.size());

	for (int i = 0; i < (int) input.size(); ++i) {
		input[i]  = makeVector(rnn_inputs[i]);
		output[i] = makeVector(rnn_expected[i]);
	}

	//Log::warn << "input : " << dump(input);
	//Log::warn << "output: " << dump(output);
}


void predict(Network *net, std::vector<Vector> const &input) {
	assert(net != nullptr);

	auto index_max = [] (Vector const &vec) -> int {
		assert(vec.rows() > 1 && vec.cols() == 1);     // Warn me if this changes
		int   index = 0;
		float max   = vec(0, 0);

	  for (int i = 1; i < (int) vec.rows(); ++i) {
			float cur = vec(i, 0);
			//Log::warn << "cur: " << cur;

			if (max < cur) {
				index = i;
				max   = cur;
			}
		}

		return index;
	};

	for (int i = 0; i < (int) input.size(); ++i) {
    Vector v = net->predict(input[i]);
	  //Log::warn << "predict " << i << ": " << dump(v);
		int max_index = index_max(v);
		//Log::warn << "max_index: " << max_index;

		std::string buf;
		switch (max_index) {
			case 0:  buf << "A"; break;
			case 1:  buf << "B"; break;
			case 2:  buf << "C"; break;
			default: buf << "UNKNOWN";
		}

	  Log::warn << "Input " << i << " is " << buf;
	}
}


/**
 * This is called `perceptron` in project `nnetcpp`.
 */
void run_rnn() {
	Log::warn << "Running run_rnn()";
	Network *net = make_perceptron();

	std::vector<Vector> input;
 	std::vector<Vector> output;
	load_data(input, output);
	
	const int iterations = 200; // 200 gives correct values!
	float train_error = trainNetwork(net, input, output, iterations, false, false, false);
	Log::warn << "run_rnn() train_error: " << train_error;

	predict(net, input);

	delete net;
}

}  // anon namespace


int main(int argc, char **argv) {
	run_rnn();
	return 0;
}
