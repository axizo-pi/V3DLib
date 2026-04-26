#include "LSTMCell.h"

LSTMCell::LSTMCell(int input_size, int hidden_size) : input_size(input_size), hidden_size(hidden_size) {
	auto width = input_size + hidden_size;

  Wf = matrix(hidden_size, width);
  bf = vector(hidden_size, 0.0f);
        
  Wi = matrix(hidden_size, width);
  bi = vector(hidden_size, 0.0f);
        
  Wc = matrix(hidden_size, width);
  bc = vector(hidden_size, 0.0f);
        
  Wo = matrix(hidden_size, width);
  bo = vector(hidden_size, 0.0f);
        
    // Initialize with random values
  for (int i = 0; i < hidden_size; i++) {
    for (int j = 0; j < input_size + hidden_size; j++) {
      Wf[i][j] = gen.rand();
      Wi[i][j] = gen.rand();
      Wc[i][j] = gen.rand();
      Wo[i][j] = gen.rand();
    }

    // Initialize biases for the forget gate to 1.0 (common practice)
    bf[i] = 1.0;
  }
}


/**
 * @brief Forward pass implementation as described in the blog
 *
 * **NOTE**: In default app, x is a vector of size 1
 */
std::pair<vector, vector> LSTMCell::forward(vector const &x, vector const &h_prev, vector const &c_prev) {
  // Store for backpropagation
  this->x_t = x;
  this->h_prev = h_prev;
  this->c_prev = c_prev;
        
  // Concatenate input and previous hidden state
  vector x_h = concat(x, h_prev);

  // Forget gate
  f_t = (Wf*x_h).sigmoid(bf);

  // Input gate
  i_t = (Wi*x_h).sigmoid(bi);

  // Cell state candidate
  c_tilde = tanh(Wc*x_h + bc);

  // Cell state update
  c_t = f_t * c_prev + i_t * c_tilde;

  // Output gate
  o_t = (Wo*x_h).sigmoid(bo);

  // Hidden state
  h_t = o_t * tanh(c_t);

	//
	// QPU
	//
	auto q_x = copy(x);                 assert(same(q_x, x));           // x is vector of size 1
	auto q_h_prev = copy(h_prev);       assert(same(q_h_prev, h_prev)); // h_prev size 32
  auto q_x_h = qpu_concat(x, h_prev); assert(same(q_x_h, x_h));

	auto q_Wf = copy(Wf);               assert(same(q_Wf, Wf));
	auto q_bf = copy(bf);               assert(same(q_bf, bf));

	auto q_Wi = copy(Wi);               assert(same(q_Wi, Wi));
	auto q_bi = copy(bi);               assert(same(q_bi, bi));

	auto q_Wc = copy(Wc);               assert(same(q_Wc, Wc));
	auto q_bc = copy(bc);               assert(same(q_bc, bc));

	auto q_c_prev = copy(c_prev);       assert(same(q_c_prev, c_prev));

	auto q_Wo = copy(Wo);               assert(same(q_Wo, Wo));
	auto q_bo = copy(bo);               assert(same(q_bo, bo));

  // Forget gate
  auto q_f_t = qpu::vector(q_Wf*q_x_h).sigmoid(q_bf);
	assert(same(q_f_t, f_t, 5.0e-7f));  // Precision for vc7

  // Input gate
  auto q_i_t = qpu::vector(q_Wi*q_x_h).sigmoid(q_bi);
	assert(same(q_i_t, i_t, 5.0e-7f));  // Precision for vc7
        
  // Cell state candidate
  auto q_c_tilde = (qpu::vector(q_Wc*q_x_h) + q_bc).tanh();
	assert(same(q_c_tilde, c_tilde, 5.0e-7f));  // Precision for vc7
        
  // Cell state update
  q_c_t = q_f_t.mul(q_c_prev) + q_i_t.mul(q_c_tilde);
	assert(same(q_c_t, c_t, 1.0e-6f));  // Precision for vc7
        
  // Output gate
  q_o_t = qpu::vector(q_Wo*q_x_h).sigmoid(q_bo);
	assert(same(q_o_t, o_t, 5.0e-7f));  // Precision for vc7
        
  // Hidden state
  auto q_h_t = q_o_t.mul(q_c_t.tanh());
	assert(same(q_h_t, h_t, 1.0e-6f));  // Precision for vc7

/*
	warn << "Wf   : " << Wf.dump_dim();
	warn << "q_x_h: " << q_x_h.dump();
*/	

	//
	// End QPU
	//
        
  return {h_t, c_t};
}


/**
 * @brief Backward pass implementation based on the blog description
 */
std::tuple<vector, vector, vector> LSTMCell::backward(
  vector const &dh_next, 
  vector const &dc_next, 
  float learning_rate, 
  float clip_value
) {
  // Backpropagation through time (BPTT)
        
  // Gradient of the output gate
  vector do_t = dh_next * tanh(c_t) * dsigmoid(o_t);
        
  // Gradient of the cell state
  vector dc_t = dc_next + dh_next * o_t * dtanh(tanh(c_t));

	//
	// QPU
	//
	auto q_dh_next = copy(dh_next);               assert(same(q_dh_next, dh_next));
	auto q_dc_next = copy(dc_next);               assert(same(q_dc_next, dc_next));
	//warn << "dh_next: " << dh_next.dump();
        
  // Gradient of the output gate
	//qpu::vector q_do_t = q_dh_next.mul(q_c_t.tanh()).mul(q_o_t.dsigmoid());
	//assert(same(q_do_t, do_t, 1.0e-6f));  // Precision for vc7
        
  // Gradient of the cell state
	//qpu::vector q_dc_t = q_dc_next + q_dh_next * q_o_t * q_c_t.tanh().dtanh();

	//
	// EndQPU
	//
        
  // Gradient of the input gate
  vector di_t = dc_t * c_tilde * dsigmoid(i_t);
        
  // Gradient of the cell state candidate
  vector dc_tilde = dc_t * i_t * dtanh(c_tilde);
        
  // Gradient of the forget gate
  vector df_t = dc_t * c_prev * dsigmoid(f_t);
        
  // Concatenate x and h_prev for weight gradient computation
  vector x_h = concat(x_t, h_prev);
        
  // Gradient clipping to prevent exploding gradients
  do_t    .clip(clip_value);
  di_t    .clip(clip_value);
  dc_tilde.clip(clip_value);
  df_t    .clip(clip_value);
        
  // Update weights using gradients
  // This is a simple implementation of gradient descent
        
  // Update forget gate weights and biases
  for (int i = 0; i < hidden_size; i++) {
    for (int j = 0; j < input_size + hidden_size; j++) {
      Wf[i][j] -= learning_rate * df_t[i] * x_h[j];
    }
    bf[i] -= learning_rate * df_t[i];
  }
        
  // Update input gate weights and biases
  for (int i = 0; i < hidden_size; i++) {
    for (int j = 0; j < input_size + hidden_size; j++) {
      Wi[i][j] -= learning_rate * di_t[i] * x_h[j];
    }
    bi[i] -= learning_rate * di_t[i];
  }
        
  // Update cell state candidate weights and biases
  for (int i = 0; i < hidden_size; i++) {
    for (int j = 0; j < input_size + hidden_size; j++) {
      Wc[i][j] -= learning_rate * dc_tilde[i] * x_h[j];
    }
    bc[i] -= learning_rate * dc_tilde[i];
  }
        
  // Update output gate weights and biases
  for (int i = 0; i < hidden_size; i++) {
    for (int j = 0; j < input_size + hidden_size; j++) {
      Wo[i][j] -= learning_rate * do_t[i] * x_h[j];
    }
    bo[i] -= learning_rate * do_t[i];
  }
        
  // Compute gradients with respect to inputs for backpropagation to earlier layers
  vector dx_t(input_size, 0.0);
  vector dh_prev(hidden_size, 0.0);
  vector dc_prev = dc_t * f_t; // Gradient of previous cell state
        
  // Gradient with respect to previous hidden state and input
  for (int i = 0; i < hidden_size; i++) {
    for (int j = 0; j < input_size; j++) {
      dx_t[j] += Wf[i][j] * df_t[i] + Wi[i][j] * di_t[i] + 
                 Wc[i][j] * dc_tilde[i] + Wo[i][j] * do_t[i];
    }

    for (int j = 0; j < hidden_size; j++) {
      dh_prev[j] += Wf[i][j + input_size] * df_t[i] + 
                    Wi[i][j + input_size] * di_t[i] + 
                    Wc[i][j + input_size] * dc_tilde[i] + 
                    Wo[i][j + input_size] * do_t[i];
    }
  }
        
  return {dx_t, dh_prev, dc_prev};
}

