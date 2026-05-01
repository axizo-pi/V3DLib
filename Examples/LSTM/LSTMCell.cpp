#include "LSTMCell.h"
#include "lstm_kernel.h"

//
// Define's to ease the transitioning to QPU
//
// LSTM will be phased out progressively.
// Actually, it is useful to retain the LSTM calculations as reference;
// Figuring out how to do this.
//
// Indications till now is that QPU converges less rapidly than LSTM.
//

//
// Do not do LSTM calculations where possible
//
//#define SKIP_LSTM_CALCULATIONS

#define USE_QPU_RESULTS

//
// End define's
//

namespace {

const float Precision_vc7 = 5.0e-7f;  // Basic precision for vc7
const float clip_value = 5.0;
    
// Random number generator for weight initialization
normal_rand gen;

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


/**
 * @brief Gradient clipping to prevent exploding gradients
 */
void Gate::clip(float clip_value, bool do_qpu) {
  d_t.clip(clip_value);

	if (do_qpu) {
  	q_d_t.clip(clip_value);
	}
  assert(same(q_d_t, d_t, Precision_vc7));
}


/**
 * @brief Check that the computations of LSTM and QPU are similar
 *
 * This call is irrelevant if either LSTM calc or QPU calc is absent.
 */
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
  q_W   = copy(W);    assert(same(q_W, W));
  q_b   = copy(m_b);  assert(same(q_b, m_b));

#ifndef SKIP_LSTM_CALCULATIONS	
  for (int i = 0; i < m_height; i++) {
    for (int j = 0; j < m_width; j++) {
      W[i][j] -= learning_rate * d_t[i] * x_h[j];
    }

    m_b[i] -= learning_rate * d_t[i];
  }
#endif // SKIP_LSTM_CALCULATIONS	

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

#ifndef SKIP_LSTM_CALCULATIONS	
  assert(same(q_W, W, Precision_vc7));
  assert(same(q_b, m_b, Precision_vc7));
#endif // SKIP_LSTM_CALCULATIONS	
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

  auto q_x      = copy(x);       assert(same(q_x, x));           // x is vector of size 1
  auto q_h_prev = copy(h_prev);  assert(same(q_h_prev, h_prev)); // h_prev size 32
  q_c_prev      = copy(c_prev);  assert(same(q_c_prev, c_prev));

  // Concatenate input and previous hidden state
  x_h   = concat(x, h_prev);
  q_x_h = qpu_concat(x, h_prev); assert(same(q_x_h, x_h));

  forget.init_forward(x_h);
  input.init_forward(x_h);
  candidate.init_forward(x_h);
  output.init_forward(x_h);

  // Cell state candidate
  c_tilde   = tanh(candidate.W*x_h + candidate.m_b);
  q_c_tilde = ((candidate.q_W*q_x_h) + candidate.q_b).tanh();
  assert(same(q_c_tilde, c_tilde, Precision_vc7));

  // Cell state update
  c_t = forget.activation * c_prev + input.activation * c_tilde;
  q_c_t = forget.q_activation.mul(q_c_prev) + input.q_activation.mul(q_c_tilde);
  assert(same(q_c_t, c_t, 2*Precision_vc7));

  // Hidden state
  h_t = output.activation * tanh(c_t);
  auto q_h_t = output.q_activation.mul(q_c_t.tanh());
  assert(same(q_h_t, h_t, 2*Precision_vc7));

	forget.check_same();
	input.check_same();
	candidate.check_same();
        
  return {h_t, c_t};
}


/**
 * @brief Gradient of the input gate
 */
void LSTMCell::gradient_input_gate() {
  input.d_t = candidate.d_t * c_tilde * dsigmoid(input.activation);

	// Size should be the same for all vectors
	int size = q_c_tilde.size();
	qpu::vector ret(size);

	gradient_input_gate_kernel().load(
  	&ret.arr(),
	  &candidate.q_d_t.arr(),
		&q_c_tilde.arr(),
		&input.q_activation.arr(),
		size >> 4
	).run();
	//warn << "q_d_t: " << input.q_d_t.dump();
	//warn << "ret  : " << ret.dump();
	//assert(same(input.q_d_t, ret, Precision_vc7));
  input.q_d_t = ret;

  assert(same(input.q_d_t, input.d_t, Precision_vc7));

  // Gradient clipping to prevent exploding gradients
  input.clip(clip_value);
}


/**
 * @brief Gradient of the output gate
 */
void LSTMCell::gradient_output_gate(vector const &dh_next, qpu::vector /* const */ &q_dh_next) {
  output.d_t = dh_next * tanh(c_t) * dsigmoid(output.activation);

	// Size should be the same for all vectors
	int size = q_dh_next.size();
	qpu::vector ret(size);

	gradient_output_gate_kernel().load(
  	&ret.arr(),
	  &q_dh_next.arr(),
		&q_c_t.arr(),
		&output.q_activation.arr(),
		size >> 4,
		clip_value
	).run();
	//warn << "q_d_t: " << output.q_d_t.dump();
	//warn << "ret  : " << ret.dump();
	//assert(same(output.q_d_t, ret)); // Compare is precise
  output.q_d_t = ret;

  output.clip(clip_value, false);
  assert(same(output.q_d_t, output.d_t, Precision_vc7));
}


/**
 * @brief Gradient of the cell state
 */
void LSTMCell::gradient_cell_state(
	vector const &dc_next,
	vector const &dh_next,
	qpu::vector /* const */ &q_dh_next
) {
  auto q_dc_next = copy(dc_next); assert(same(q_dc_next, dc_next));

  candidate.d_t = dc_next + dh_next * output.activation * dtanh(tanh(c_t));

	// Size should be the same for all vectors
	int size = q_dc_next.size();
	qpu::vector ret(size);

	gradient_cell_state_kernel().load(
  	&ret.arr(),
		&q_dc_next.arr(),
	  &q_dh_next.arr(),
		&output.q_activation.arr(),
		&q_c_t.arr(),
		size >> 4
	).run();

  candidate.q_d_t = ret;
  assert(same(candidate.q_d_t, candidate.d_t, 2*Precision_vc7));
}


/**
 * @brief Gradient of the cell state candidate
 */
void LSTMCell::gradient_candidate() {
  candidate.d_t = candidate.d_t * input.activation * dtanh(c_tilde);

	// Size should be the same for all vectors
	int size = q_c_tilde.size();
	qpu::vector ret(size);

	// Note that output is also an input; take care here
	gradient_candidate_kernel().load(
  	&ret.arr(),
		&candidate.q_d_t.arr(),
		&input.q_activation.arr(),
		&q_c_tilde.arr(),
		size >> 4,
		clip_value
	).run();
  candidate.q_d_t = ret;

  candidate.clip(clip_value, false);
  assert(same(candidate.q_d_t, candidate.d_t, Precision_vc7));
}


/**
 * @brief Gradient of the forget gate
 */
void LSTMCell::gradient_forget_gate() {
  forget.d_t = candidate.d_t * c_prev * dsigmoid(forget.activation);

	// Size should be the same for all vectors
	int size = q_c_prev.size();
	qpu::vector ret(size);

	// Note that output is also an input; take care here
	gradient_forget_kernel().load(
  	&ret.arr(),
		&candidate.q_d_t.arr(),
		&q_c_prev.arr(),
		&forget.q_activation.arr(),
		size >> 4,
		clip_value
	).run();
  forget.q_d_t = ret;

  forget.clip(clip_value, false);
  assert(same(forget.q_d_t, forget.d_t, Precision_vc7));
}


/**
 * @brief Backward pass implementation based on the blog description
 *
 * Backpropagation through time (BPTT)
 */
std::tuple<vector, vector, vector> LSTMCell::backward(
  vector const &dh_next, 
  vector const &dc_next, 
  float learning_rate
) {
  auto q_dh_next = copy(dh_next);

  // Interim copies to circumvent accumulating errors
  q_c_t = copy(c_t);  assert(same(q_c_t, c_t, Precision_vc7));

	gradient_output_gate(dh_next, q_dh_next);
	gradient_cell_state(dc_next, dh_next, q_dh_next);
	gradient_input_gate();
	gradient_candidate();
	gradient_forget_gate();

  //  
  // Update weights using gradients
  // This is a simple implementation of gradient descent
  //

  //
  // Update forget gate weights and biases
  forget.update_gate(x_h, q_x_h, learning_rate);

	//
	// Specific for forget gate:
	//
  // For some reason, _only_ element (3,0) (row, column) of q_Wi is different.
  // All the rest are in effect identical.
  // I suspect that there is some subtle bug in the update_gate kernel.
	//
  assert(same(input.q_W, input.W, 1.0e-2f));
  assert(same(input.q_b, input.m_b, Precision_vc7));

  input.update_gate(x_h, q_x_h, learning_rate);
  candidate.update_gate(x_h, q_x_h, learning_rate);
  output.update_gate(x_h, q_x_h, learning_rate);
    
	//	
  // Compute gradients with respect to inputs for backpropagation to earlier layers
	//
  vector dc_prev = candidate.d_t * forget.activation; // Gradient of previous cell state

#ifndef SKIP_LSTM_CALCULATIONS	
  vector dx_t(input_size, 0.0);
  vector dh_prev(hidden_size, 0.0);
        
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
#endif // SKIP_LSTM_CALCULATIONS	

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

	lstm::vector q_dx_t    = copy(result, 0, input_size);
	lstm::vector q_dh_prev = copy(result, input_size, hidden_size);

/*	
	warn << "dx_t  : " << dx_t.dump();
	warn << "q_dx_t: " << q_dx_t.dump();
	warn << "dh_prev  : " << dh_prev.dump();
	warn << "q_dh_prev: " << q_dh_prev.dump();
	//warn << "result: " << result.dump();
*/	

#ifndef SKIP_LSTM_CALCULATIONS	
	assert(same(dx_t, q_dx_t, Precision_vc7));
	assert(same(dh_prev, q_dh_prev, Precision_vc7));
#endif // SKIP_LSTM_CALCULATIONS	
        
#ifdef USE_QPU_RESULTS
  return {q_dx_t, q_dh_prev, dc_prev};
#else
  return {dx_t, dh_prev, dc_prev};
#endif // USE_QPU_RESULTS
}

