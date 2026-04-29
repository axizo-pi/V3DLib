#ifndef _LSTM_LSTMCELL_H
#define _LSTM_LSTMCELL_H
#include "lstm_matrix.h"
#include "helpers.h"
#include "convert.h"
#include <tuple>

using namespace lstm;

class Gate {
private:
	int m_height;	
	int m_width;	

public:	
	Gate(int height, int width, float default_bias = 0.0f);

	void init_forward(vector const &x_h);
	void clip(float clip_value);
	void update_gate(vector const &x_h, qpu::vector const &q_x_h, float learning_rate);

  // Weights and biases
  matrix W;
  vector m_b;         // gate bias

  vector activation;

	// QPU
	qpu::matrix q_W;
	qpu::vector q_b;
	qpu::vector q_activation;

	// Following should actually be local to forward()
  vector d_t;         // Gradient of the gate; for candidate it was called dc_tilde
	qpu::vector q_d_t;
};


/**
 * @brief LSTM Cell implementation as described in the blog
 */
class LSTMCell {
private:
  int input_size;
  int hidden_size;

	Gate forget;
	Gate input;
	Gate candidate;  // Cell state candidate
	Gate output;
    
  // For backpropagation
  vector x_t;        // Input at time t
  vector h_prev;     // Previous hidden state
  vector x_h;        // Concatenated x_t and h_prev
  vector c_prev;     // Previous cell state
  vector c_tilde;    // Cell state candidate
  vector c_t;        // Current cell state
  //vector o_t;        // Output gate activation
  vector h_t;        // Current hidden state

	//
	// QPU
	//
	qpu::vector q_c_t;
	//qpu::vector q_o_t;
	qpu::vector q_c_tilde;
	qpu::vector q_c_prev;
	qpu::vector q_x_h;

	// End QPU

public:
	LSTMCell(int input_size, int hidden_size);

  std::pair<vector, vector> forward(vector const &x, vector const &h_prev, vector const &c_prev);
  std::tuple<vector, vector, vector> backward(vector const &dh_next, vector const &dc_next,
			                                        float learning_rate, float clip_value = 5.0);
};

#endif // _LSTM_LSTMCELL_H
