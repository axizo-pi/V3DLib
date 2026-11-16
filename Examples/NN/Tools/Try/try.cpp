#include "file.h"        // This and cpp from GRU
#include <network.h>     // This and next from nnetcpp
#include <dense.h>
#include <activation.h>
#include <utils.h>       // from nnetcpp tests
#include "rnn_data.h"    // NOTE: this and cpp file copied from RNN
#include "global/Log.h"  // this and next from V3DLib
#include "global/debug.h"
#include "global/string.h"
#include <string>        // globals
#include <iostream>

using namespace std;
using namespace Log;

namespace {

using Sequence  = std::vector<Vector>;
using Sequences = std::vector<std::vector<Vector>>;

//
// Model dimensions
//	
// Dimensions must match with input file
//
const int input_dim       = 64;
const int hidden_dim      = 64;
const int output_dim      = 64;
const float learning_rate = 0.0005f;
const int time_steps      = 10;       //> I guess this is the same as epochs in other example apps
const int iterations      = 2000;     //> # iterations when doing random iterations

// GRU
const int  seq_length        = input_dim;
const bool random_iterations = false;


vector<int> slice(vector<int> &arr, int X, int len) {
  // Starting and Ending iterators
  auto start = arr.begin() + X;
  auto end   = arr.begin() + X + len;

  // To store the sliced vector
  vector<int> result(len);

	// Copy vector using copy function()
  copy(start, end, result.begin());

  // Return the final sliced vector
  return result;
}


std::string dump(Vector const &vec) {
  std::stringstream buf;

  buf << "(rows, cols): (" << vec.rows() << ", " << vec.cols() << ") transposed: "
      << "<" << vec.transpose() << ">";

  return buf.str();
}


std::string dump(Sequence const &input, bool do_transposed = true) {
  assert(!input.empty());
  std::stringstream buf;
  auto const &vec = input[0];

	if (do_transposed) {
    buf << "Size: " << (int) input.size() << ", "
        << "(rows, cols): (" << vec.rows() << ", " << vec.cols() << ")"
        << "\nItems transposed:";
	}

  for (int i = 0; i < (int) input.size(); ++i) {
    auto const &cur = input[i];
    assert(cur.rows() == vec.rows() && cur.cols() == vec.cols());

		if (do_transposed) {
    	buf << "\n  " << i << ": <" << cur.transpose() << ">";
		} else {
    	buf << cur << ", ";
		}
  }

  return buf.str();
}


std::string dump_dim(Sequences const &inputs) {
  assert(!inputs.empty());
  std::stringstream buf;

	buf << "size : " << inputs.size()  << ", "
		  << "elements size: " << inputs[0].size() << ", "
			<< "element(r,c): ("
			<< (int) inputs[0][0].rows() << ", "
			<< (int) inputs[0][0].cols() << ")";

  return buf.str();
}


std::string dump_dim(Vector const &v) {
  std::stringstream buf;

	buf << " (r,c): ("
			<< (int) v.rows() << ", "
			<< (int) v.cols() << ")";

  return buf.str();
}


std::string dump_dim(Sequence const &inputs) {
  assert(!inputs.empty());
  std::stringstream buf;

	buf << "size : " << inputs.size()  << ", "
		  << "element size: " << inputs[0].size() << ", "
			<< "inputs[0]: " << dump_dim(inputs[0]);

  return buf.str();
}


std::string dump(Sequences const &inputs) {
  assert(!inputs.empty());
  std::stringstream buf;
  auto const &vec  = inputs[0];
  auto const &vec2 = vec[0];

  buf << "Inputs Size: " << (int) inputs.size() << ", "
      << "elements Size: " << (int) vec.size() << ", "
      << "(rows, cols): (" << vec2.rows() << ", " << vec2.cols() << ")\n";

  for (int i = 0; i < (int) inputs.size(); ++i) {
		buf << i << ": " << dump(inputs[i], false) << "\n";
	}

  return buf.str();
}


/**
 * @brief Train the network on the input sequences (all but the last one)
 *
 * Used by `GRU` and `LSTM`.
 */
void train_loop(Network *net, Sequences const &inputs, Sequences const &outputs, int steps) {
	auto display = [] (int step, int steps) -> bool {
		if (steps >= 500) {
			return step % 100 == 0;
		} else if (steps >= 200) {
			return step % 50 == 0;
		} else if (steps >= 100) {
			return step % 20 == 0;
		} else {
			return true;
		}
	};

  for (int step = 0; step < steps; ++step) {

  	float target_mse = 0.0f;
    int size = ((int) inputs.size() - 1);

		if (random_iterations) {
      for (int iteration = 0; iteration < iterations; ++iteration) {
        int i = rand() % size;
        assert(i < (int) inputs.size() - 1);
        target_mse += trainNetwork(net, inputs[i], outputs[i], 1, false, true, false);
      }

			// Take average MSE over iterations, for better comparison
			target_mse /= iterations;
		} else {
      for (int j = 0; j < size; ++j) {
     	  target_mse += trainNetwork(net, inputs[j], outputs[j], 1, false, false, false);
		  }

			target_mse /= size;
		}

		if (display(step, steps)) {
		  warn << "======= step: " << step << ",  mse: " << target_mse;
		}
  }
}


Sequence do_predict(Network *net, Sequence const &n) {
	Sequence s;
	for (int i = 0 ; i < (int) n.size(); ++i) {
    Vector v = net->predict(n[i]);
	  //warn << "predict v: " << dump(v);
		s.push_back(v);
	}

	return s;
}


namespace rnn_try {

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


void load_data(Sequence &input, Sequence &output) {
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
void run() {
  Log::warn << "Running RNN";
  Network *net = make_perceptron();

  Sequence input;
  Sequence output;
  load_data(input, output);
  
  const int iterations = 200; // 200 gives correct values!
  float train_error = trainNetwork(net, input, output, iterations, false, false, false);
  Log::warn << "rnn_try::run() train_error: " << train_error;

  predict(net, input);

  delete net;
}

}  // namespace rnn_try


namespace gru {

void to_float(std::vector<float> &out, std::vector<int> in) {
  out.resize(in.size());

  for (int i = 0; i < (int) in.size(); ++i) {
    out[i] = (float) in[i];
  }
}


int max_index(Vector const &v) {
	assert(v.cols() == 1);

	int index = 0;
	float max = v(0, 0);
	for (int i = 1; i < v.rows(); ++i) {
		float val = v(i, 0);
		if (max < val) {
			max = val;
			index = i;
		}
	}

	return index;
}


/**
 * @brief Convert a Vector sequence to a string
 */
std::string to_string(Sequence const &seq) {
	assert(!seq.empty());
	assert(seq[0].cols() == 1);

	std::vector<int> max_indexes;
	for (int i = 0; i < seq.size(); ++i) {
		max_indexes.push_back(max_index(seq[i]));
	}
	assert(max_indexes.size() == seq.size());

	return index_to_s(max_indexes);
}


void indexes_to_sequence(Sequence &seq, std::vector<int> &indexes) {
	//warn << "indexes_to_sequence indexes size: " << indexes.size();
	assert((int) indexes.size() <= input_dim);  // Deal with this case when it happens

	std::vector<float> floats(input_dim); //indexes.size());
	for (int i = 0; i < (int) indexes.size(); ++i) {
		//warn << "index: " << indexes[i];
		std::fill(floats.begin() , floats.end() , 0);
		floats[indexes[i]] = 1;
	  seq.push_back(makeVector(floats));
	}
}


void load_data(Sequences &inputs, Sequences &outputs) {
  std::string path = "../../GRU/Tools/GRU/Inputs/";

  std::string infile  = path + "donald_trump_input.txt";
  std::string outfile = path + "donald_trump_output.txt";
  //warn << "infile: " << infile;

  std::vector<int> x_in_int;
  std::vector<int> x_out_int;

  load_x_y(infile, outfile, x_in_int, x_out_int);

	inputs.clear();
	outputs.clear();

	int num_sequences = (int) x_in_int.size()/seq_length;
	warn << "num_sequences: " << num_sequences;
	assert(seq_length % input_dim == 0);
	assert(num_sequences*seq_length < (int) x_in_int.size());

  Sequence seq;
	
	for (int n = 0; n < num_sequences; ++n) {
		seq.clear();
		for (int i = 0; i < seq_length; i += input_dim) {
		  int x = n*seq_length + i;
		  auto in_vec  = slice(x_in_int , x, input_dim);
		  indexes_to_sequence(seq, in_vec);
		}
	  inputs.push_back(seq);

		seq.clear();
		for (int i = 0; i < seq_length; i += output_dim) {
		  int x = n*seq_length + i;
		  auto out_vec = slice(x_out_int, x, output_dim);
		  indexes_to_sequence(seq, out_vec);
		}
		outputs.push_back(seq);
	}

	//warn << "load_data inputs.size(): " << inputs.size();
	assert(inputs.size()  == num_sequences);
	assert(outputs.size() == num_sequences);
}


void do_predict(Network *net, Sequence const &n) {
	Sequence s = ::do_predict(net, n);
	warn << "predict n: '" << to_string(n) << "'";
	warn << "predict s: '" << to_string(s) << "'";
}


void do_predict(Network *net, Sequences const &seqs, int num_seqs = 1 /* 5 */) {
	// Last sequence included
	for (int i = 0; i < num_seqs; ++i) {
  	do_predict(net, seqs[i]);
	}
}


void do_predict(Network *net, std::string const &str) {
	assert((int) str.size() < seq_length);  // Deal with this case when it happens
	auto predict_str = str;
	//auto predict_str = string::pad(str, input_dim); //, false);
	//warn << "predict_input: '" << predict_str << "'";

	std::vector<int> indexes = s_to_index(predict_str); 
	//warn << "indexes size: " << indexes.size();

	Sequence predict_seq;
	indexes_to_sequence(predict_seq, indexes);
	//warn << "predict_seq input size: " << (int) predict_seq.front().rows();
	//warn << "predict_seq input size: " << (int) predict_seq.back().rows();
  //warn << "net input port size   : " << (int) net->inputPort()->value.size();

	//warn << "predict_seq: '" << to_string(predict_seq) << "'";
	//warn << "predict_seq: " << dump(predict_seq);
	do_predict(net, predict_seq);
}


/**
 * Current hypothesis:
 *
 * - The network has 64 inputs, one for each possible character
 * - A sequence consists of a list of Vector's, in which exaclty one input is set
 *
 * Consider using words as tokens; should work much more effective.
 * Or vowels.
 *
 * ----------------------------------------------
 *
 * Trial runs
 * ==========
 *
 * Defaults:
 * - Parameters not mentioned have default values
 * - random iterations
 *
 * 1. Default run
 *
 * ======= step: 99,  mse: 1.182099e-02
 * predict n: 'thank you to @exxonmobil for your $20 billion investment that is'
 * predict s: 'henk tou th arriar erenltor t u  ar00tell nn  n er  e t ahen on '
 * predict n: 'i am the best'
 * predict s: 'nwneihe te t '
 * 
 * real	7m21.386s
 *
 * 2. All iterations
 *
 * ======= step: 99,  mse: 1.150639e-02
 * predict n: 'thank you to @exxonmobil for your $20 billion investment that is'
 * predict s: 'retd you th tr ann arlnlaor aou  tn 0tetl nn tn ir  e   then in '
 * predict n: 'i am the best'
 * WARNING: predict s: 'nanerhe ter h'
 * 
 * real	19m7.274s
 *
 * - Slower but better convergence
 *
 * 2. Double hidden_dim = 128
 *
 * ======= step: 99,  mse: 1.138175e-02
 * predict n: 'thank you to @exxonmobil for your $20 billion investment that is'
 * predict s: 'henk you th trnairainanltor tou  ar00sell nn in ertient ohen  n '
 * predict n: 'i am the best'
 * predict s: 'nn erhe se t '
 * 
 * real	18m56.843s
 * 
 *
 * ----------------------------------------------
 * Old Runs (discard?)
 * --------
 *
 * - const int hidden_dim      = 128;
 * - const int time_steps      = 100;
 * - double seq_length
 * - All iterations
 * - rest default
 * 
 * ======= step: 99,  mse: 2.810924e+01
 * predict n: 'thank you to @exxonmobil for your $20 billion investment that is creating more than 45,000 manufacturing & construction jobs in '
 * predict s: 'een  aou th trn in arsnlior tou  ao01iellenn an ert ert then in tooateog aere ahenkte  01 iakalolt rstg a tonairumo ln tois tn t'
 * predict n: '                                                   i am the best'
 * predict s: 'ttttttttttttttttttttttttttttttttttttttttttttttttttt lr iheroett '
 *
 *
 * - const int time_steps      = 500;
 * - double seq_length
 * - All iterations
 * - rest default
 *
 * ======= step: 499,  mse: 2.924953e+01
 * predict n: 'thank you to @exxonmobil for your $20 billion investment that is creating more than 45,000 manufacturing & construction jobs in '
 * predict s: 'h ng you th trn  u arslltor tou  to00till nn in er  e   ihet in tooal og tare thetg o  0  tardsoleii  g t iongt umtion ioil tn t'
 * predict n: '                                                   i am the best'
 * predict s: 'ttttttttttttttttttttttttttttttttttttttttttttttttttt tn ihe ae t '
 * 
 * real	93m57.078s
 * user	93m56.468s
 * sys	0m0.032s
 *
 * - Slow convergence, sometimes goes up (but it _does_ converge).
 *
 *
 * - const int time_steps      = 500;
 * - double seq_length
 * - random iterations
 * - rest default
 */
void run() {
  Log::warn << "Running GRU";

  Sequences inputs;
  Sequences outputs;
  load_data(inputs, outputs);
	warn << "inputs  " << dump_dim(inputs);
	warn << "outputs " << dump_dim(outputs);

	Network *net = makeGRU(input_dim, hidden_dim, output_dim, learning_rate);
	warn << "elements input size: " << (int) inputs[0][0].rows();
  warn << "net input port size: " << (int) net->inputPort()->value.size();

	train_loop(net, inputs, outputs, time_steps);

	net->reset();
	do_predict(net, inputs);
	do_predict(net,"i am the best");

	delete net;
}

}  // namespace gru


namespace lstm {

const int input_size       = 1;
const int hidden_size      = 2*32;
const int output_size      = 1;
const double learning_rate = 0.01f;
const int epochs           = 3000; //50;

struct Data {

Sequence input;
Sequence expected;

Data(int size) {
	vector<float> f_input;
	vector<float> f_expected;

  // Create a simple sine wave dataset for demonstration
  for (int i = 0; i < size; i++) {
    float x = i * 0.1;
    f_input.push_back(x);
    f_expected.push_back(sin(x));
  }

	input    = makeSequence(f_input);
	expected = makeSequence(f_expected);

	//warn << "input   : "<< dump(input, false);
	//warn << "expected: "<< dump(expected, false);
}

};


void run() {
	warn << "Doing lstm::run()";
	Data data(100);

  Network *net = makeLSTM(input_size, hidden_size, output_size, learning_rate);
	train_loop(net, {data.input, {}}, {data.expected}, epochs);
  //const int iterations = 8000; //200;
  //float train_error = trainNetwork(net, data.input, data.expected, iterations, false, true, false);
  //Log::warn << "rnn_try::run() train_error: " << train_error;

  net->reset();

	Data check(10);
	Sequence predicted = ::do_predict(net, check.input);
	//warn << "predict : "<< dump(predicted, false);
	//warn << "expected: "<< dump(expected, false);

	int count = 0;
	std::string buf;
	for (auto const &v: predicted) {
    buf << count << ": " << v[0] << ", expected: " << check.expected[count][0] << "\n";
		count++;
	}
	warn << "buf:\n" << buf;

	delete net;
}

}  // namespace lstm
}  // anon namespace


int main(int argc, char **argv) {
  //rnn_try::run();
  //gru::run();
  lstm::run();
  return 0;
}
