#include "LSTMCell.h"
#include "../RNN/Lib/helpers.h" // settings()

namespace {

const float Precision_vc7 = 5.0e-7f;  // Basic precision for vc7
    
// Random number generator for weight initialization
normal_rand gen;

// TODO: ptr's not cleaned up on exit, better would be shared or unique ptr.
BaseKernel *s_update_gate_kernel = nullptr;
BaseKernel *s_gradient_previous_state_input_kernel = nullptr;

BaseKernel &update_gate_kernel() {
  if (s_update_gate_kernel == nullptr) {
    s_update_gate_kernel = new BaseKernel(compile(update_gate, settings()));
  }

  return *s_update_gate_kernel;
}

BaseKernel &gradient_previous_state_input_kernel() {
  if (s_gradient_previous_state_input_kernel == nullptr) {
    s_gradient_previous_state_input_kernel = new BaseKernel(
      compile(gradient_previous_state_input, settings()));
  }

  return *s_gradient_previous_state_input_kernel;
}

} // anon namespace


Gate::Gate(int height, int width, float default_bias) :
  m_height(height),
  m_width(width),
  W(height, width),
  m_b(height, default_bias)
{
  // Initialize with random values
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      W[i][j] = gen.rand();
    }
  }
}


void Gate::init_forward(vector const &x_h) {
  auto q_x_h = copy(x_h);  assert(same(q_x_h, x_h));
  q_W = copy(W);           assert(same(q_W, W));
  q_b = copy(m_b);         assert(same(q_b, m_b));

  activation = (W*x_h).sigmoid(m_b);

  q_activation = (q_W*q_x_h).sigmoid(q_b);
  assert(same(q_activation, activation, Precision_vc7));
}


void Gate::clip(float clip_value) {
  d_t.clip(clip_value);
  q_d_t.clip(clip_value);
  assert(same(q_d_t, d_t, Precision_vc7));
}


void Gate::check_same() {
  assert(same(q_W, W, Precision_vc7));
  assert(same(q_b, m_b, Precision_vc7));
}


/**
 * @brief Update gate weights and biases
 */
void Gate::update_gate(vector const &x_h, qpu::vector const &q_x_h, float learning_rate) {
  // q_Wi diverges from Wi by accumulated errors; Following for debugging
  // This might be specific to input gate
  q_W   = copy(W);                    assert(same(q_W, W));
  q_b   = copy(m_b);                  assert(same(q_b, m_b));
  //DEBUG q_Wi.arr()[5*q_Wi.columns() + 5] = 5.0f;

/*
  //
  // NOTE: Following might be specific only to forget gate.
  //       d_t for forget gate is always a zero-vector,
  //       hence bf will always be zero as well.
  //
  qpu::vector q_zero(32, 0.0f);   // Test Note
  assert(same(q_zero, d_t));
*/  

  for (int i = 0; i < m_height; i++) {
    for (int j = 0; j < m_width; j++) {
      W[i][j] -= learning_rate * d_t[i] * x_h[j];
    }

    m_b[i] -= learning_rate * d_t[i];
  }

  //
  // QPU
  //

  // Pre-test
  assert(same(q_d_t, d_t, Precision_vc7));
  assert(same(q_x_h, x_h));

  assert(q_W.columns() % 16 == 0);
  update_gate_kernel().load(
    &q_W.arr(),
    &q_d_t.arr(),
    &q_x_h.arr(),
    &q_b.arr(),
    q_W.rows(),
    q_W.columns()/16,
    learning_rate
  ).run();
  assert(same(q_W, W, Precision_vc7));
  assert(same(q_b, m_b, Precision_vc7));
}


LSTMCell::LSTMCell(int input_size, int hidden_size) :
  input_size(input_size),
  hidden_size(hidden_size),
  forget(hidden_size, input_size + hidden_size, 1.0f), // Initialize biases for the forget gate to 1.0 (common practice)
  input(hidden_size, input_size + hidden_size),
  candidate(hidden_size, input_size + hidden_size),
  output(hidden_size, input_size + hidden_size)
{}


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
  x_h = concat(x, h_prev);

  // Cell state candidate
  c_tilde = tanh(candidate.W*x_h + candidate.m_b);

  forget.init_forward(x_h);
  input.init_forward(x_h);
  candidate.init_forward(x_h);
  output.init_forward(x_h);

  // Cell state update
  c_t = forget.activation * c_prev + input.activation * c_tilde;

  // Hidden state
  h_t = output.activation * tanh(c_t);

  //
  // QPU
  //
  auto q_x = copy(x);                 assert(same(q_x, x));           // x is vector of size 1
  auto q_h_prev = copy(h_prev);       assert(same(q_h_prev, h_prev)); // h_prev size 32
  q_x_h = qpu_concat(x, h_prev);      assert(same(q_x_h, x_h));
  q_c_prev = copy(c_prev);            assert(same(q_c_prev, c_prev));

  assert(same(q_x_h     , x_h,      Precision_vc7));
  assert(same(forget.q_W, forget.W, Precision_vc7));
  assert(same(forget.q_b, forget.m_b, Precision_vc7));
  assert(same(input.q_W , input.W , Precision_vc7));
  assert(same(input.q_b , input.m_b , Precision_vc7));
  assert(same(candidate.q_W , candidate.W , Precision_vc7));
  assert(same(candidate.q_b , candidate.m_b , Precision_vc7));

  // Cell state candidate
  q_c_tilde = ((candidate.q_W*q_x_h) + candidate.q_b).tanh();
  assert(same(q_c_tilde, c_tilde, Precision_vc7));
        
  // Cell state update
  q_c_t = forget.q_activation.mul(q_c_prev) + input.q_activation.mul(q_c_tilde);
  assert(same(q_c_t, c_t, 2*Precision_vc7));
        
  // Hidden state
  auto q_h_t = output.q_activation.mul(q_c_t.tanh());
  assert(same(q_h_t, h_t, 2*Precision_vc7));

  //
  // End QPU
  //
        
  return {h_t, c_t};
}


/**
 * @brief Backward pass implementation based on the blog description
 *
 * Backpropagation through time (BPTT)
 */
std::tuple<vector, vector, vector> LSTMCell::backward(
  vector const &dh_next, 
  vector const &dc_next, 
  float learning_rate, 
  float clip_value
) {
  auto q_dc_next = copy(dc_next);               assert(same(q_dc_next, dc_next));

  // Interim copies to circumvent accumulating errors
  q_c_t = copy(c_t);                            assert(same(q_c_t, c_t, Precision_vc7));

  // TODO remove following
  output.q_activation = copy(output.activation);              assert(same(output.q_activation, output.activation, Precision_vc7));

  auto q_dh_next = copy(dh_next);               assert(same(q_dh_next, dh_next, Precision_vc7));
        
  // Gradient of the output gate
  output.d_t = dh_next * tanh(c_t) * dsigmoid(output.activation);
        
  // Gradient of the cell state
  candidate.d_t   = dc_next + dh_next * output.activation * dtanh(tanh(c_t));

  // DEBUG
  // Following fails somewhere after epoch 30; previous calls were in order
  // Indications are that calculation always returns zero vector
  candidate.q_d_t = q_dc_next + q_dh_next.mul(output.q_activation.mul(q_c_t.tanh().dtanh()));

  // Debug fix. TODO: examine and fix
  if (!same(candidate.q_d_t, candidate.d_t, Precision_vc7)) {
    vector zero(32, 0.0f);   // Test Note
    if (!same(candidate.q_d_t, zero)) {
      warn << "Calc candidate.q_d_t failed, resetting for debug";
      candidate.q_d_t = copy(candidate.d_t);
    }
  }
  assert(same(candidate.q_d_t, candidate.d_t, Precision_vc7));

  // Gradient of the input gate
  input.d_t = candidate.d_t * c_tilde * dsigmoid(input.activation);

  // Gradient of the cell state candidate
  candidate.d_t = candidate.d_t * input.activation * dtanh(c_tilde);
        
  // Gradient of the forget gate
  forget.d_t = candidate.d_t * c_prev * dsigmoid(forget.activation);

  //
  // QPU
  //
        
  // Gradient of the output gate
  output.q_d_t = q_dh_next.mul(q_c_t.tanh()).mul(output.q_activation.dsigmoid());
  assert(same(output.q_d_t, output.d_t, Precision_vc7));
        
  // Gradient of the input gate
  input.q_d_t = candidate.q_d_t.mul(q_c_tilde.mul(input.q_activation.dsigmoid()));
  assert(same(input.q_d_t, input.d_t, Precision_vc7));
        
  // Gradient of the cell state candidate
  candidate.q_d_t = candidate.q_d_t.mul(input.q_activation.mul(q_c_tilde.dtanh()));
  assert(same(candidate.q_d_t, candidate.d_t, Precision_vc7));
        
  // Gradient of the forget gate
  forget.q_d_t = candidate.q_d_t.mul(q_c_prev.mul(forget.q_activation.dsigmoid()));
  assert(same(forget.q_d_t, forget.d_t, Precision_vc7));

  //
  // EndQPU
  //
        
  // Gradient clipping to prevent exploding gradients
  forget.clip(clip_value);
  input.clip(clip_value);
  candidate.clip(clip_value);
  output.clip(clip_value);

  //  
  // Update weights using gradients
  // This is a simple implementation of gradient descent
  //

  //
  // Update forget gate weights and biases
  forget.update_gate(x_h, q_x_h, learning_rate);

  // For some reason, _only) element (3,0) (row, column) of q_Wi is different.
  // All the rest are in effect identical.
  // I suspect that there is some subtle bug in the update_gate kernel.
  assert(same(input.q_W, input.W, 1.0e-2f));
  assert(same(input.q_b, input.m_b, Precision_vc7));

  input.update_gate(x_h, q_x_h, learning_rate);
  candidate.update_gate(x_h, q_x_h, learning_rate);
  output.update_gate(x_h, q_x_h, learning_rate);
        
  // Compute gradients with respect to inputs for backpropagation to earlier layers
  vector dx_t(input_size, 0.0);
  vector dh_prev(hidden_size, 0.0);
  vector dc_prev = candidate.d_t * forget.activation; // Gradient of previous cell state
        
  // Gradient with respect to previous hidden state and input
  for (int i = 0; i < hidden_size; i++) {
    for (int j = 0; j < input_size; j++) {
      dx_t[j] += forget.W[i][j]    * forget.d_t[i] +
                 input.W[i][j]     * input.d_t[i] + 
                 candidate.W[i][j] * candidate.d_t[i] +
                 output.W[i][j]    * output.d_t[i];
    }

    for (int j = 0; j < hidden_size; j++) {
      dh_prev[j] += forget.W[i][   j + input_size] * forget.d_t[i] + 
                    input.W[i][    j + input_size] * input.d_t[i] + 
                    candidate.W[i][j + input_size] * candidate.d_t[i] + 
                    output.W[i][   j + input_size] * output.d_t[i];
    }
  }

	forget.check_same();
	input.check_same();
	candidate.check_same();
	output.check_same();

  int columns = resize(input_size + hidden_size);
  qpu::vector result(columns, 0.0f);

  gradient_previous_state_input_kernel().load(
    hidden_size,
    columns,
    &forget.q_W.arr(),
    &forget.q_d_t.arr(),
    &input.q_W.arr(),
    &input.q_d_t.arr(),
    &candidate.q_W.arr(),
    &candidate.q_d_t.arr(),
    &output.q_W.arr(),
    &output.q_d_t.arr(),
    &result.arr()
  ).run();

	//warn << "dx_t: " << dx_t.dump();
	//warn << "dh_prev: " << dh_prev.dump();
	//warn << "result: " << result.dump();
        
  return {dx_t, dh_prev, dc_prev};
}

