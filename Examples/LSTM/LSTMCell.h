#ifndef _LSTM_LSTMCELL_H
#define _LSTM_LSTMCELL_H
#include "lstm_matrix.h"
#include "helpers.h"
#include "convert.h"
#include <tuple>

using namespace lstm;

/**
 * @brief LSTM Cell implementation as described in the blog
 */
class LSTMCell {
private:
  int input_size;
  int hidden_size;
    
  // Weights and biases
  matrix Wf;         // Forget gate weights
  vector bf;         // Forget gate bias
  matrix Wi;         // Input gate weights
  vector bi;         // Input gate bias

  matrix Wc;         // Cell state candidate weights
  vector bc;         // Cell state candidate bias
  matrix Wo;         // Output gate weights
  vector bo;         // Output gate bias
    
  // For backpropagation
  vector x_t;        // Input at time t
  vector h_prev;     // Previous hidden state
  vector c_prev;     // Previous cell state
  vector f_t;        // Forget gate activation
  vector i_t;        // Input gate activation
  vector c_tilde;    // Cell state candidate
  vector c_t;        // Current cell state
  vector o_t;        // Output gate activation
  vector h_t;        // Current hidden state
    
  // Random number generator for weight initialization
	normal_rand gen;

	//
	// QPU
	//
	qpu::vector q_c_t{32};      // Dummy init value
	qpu::vector q_o_t{32};      // " etc
	qpu::vector q_i_t{32};
	qpu::vector q_c_tilde{32};
	qpu::vector q_c_prev{32};
	qpu::vector q_f_t{32};

	//
	// End QPU
	//

public:
	LSTMCell(int input_size, int hidden_size);

  std::pair<vector, vector> forward(vector const &x, vector const &h_prev, vector const &c_prev);
  std::tuple<vector, vector, vector> backward(vector const &dh_next, vector const &dc_next,
			                                        float learning_rate, float clip_value = 5.0);
};

#endif // _LSTM_LSTMCELL_H
