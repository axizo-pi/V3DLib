#include "lstm_kernel.h"
#include "V3DLib.h"
#include "../Lib/helpers.h"  // settings()
#include "kernels.h"         // clip_partial()

using namespace V3DLib;

/**
 * /file
 *
 * Kernels specific for LSTM.
 *
 * The goal is to combine basic kernel operations in an encompassing kernel,
 * to reduce kernel calls and loop overhead.
 */


namespace {

/**
 * @brief Set all values in vector to value at index n
 */
void set_all(Float &input, Float &result, Int n) {
  Float tmp = 0.0f;
  set_at(tmp, n, input);
  rotate_sum(tmp, result);
}


/**
 * @brief Update gate weights and biases
 *
 * @param         W weight matrix, input/output 
 * @param columns Number of columns, dimension in blocks of 16
 * @param bias    bias vector, output only
 */
void update_gate(
  Float::Ptr W,
  Float::Ptr in_d_t,
  Float::Ptr x_h,
  Float::Ptr bias,
  Int rows,
  Int columns,
  Float learning_rate
) {
  Int stride = 16*4*columns;
  Float::Ptr d_t = in_d_t;
  Float d_t_val = *d_t;

  For (Int i = 0, i < rows, i++)
    Float::Ptr w_start = W.offset(i*stride);
    Float::Ptr x_h_start = x_h;

    If (i > 0 && ((i & 0xf) == 0))
      d_t.inc();
      d_t_val = *d_t;
    End

    Float d_t_i;
    set_all(d_t_val, d_t_i, (i & 0xf));
  
    For (Int j = 0, j < columns, j++)
      Float tmp = *w_start;
      tmp -= (learning_rate * d_t_i * (*x_h_start));
      *w_start = tmp;

      w_start.inc();
      x_h_start.inc();
    End
  End

  d_t = in_d_t;
  Int count = rows >> 4;
  For (Int i = 0, i < count, i++)
    Float tmp = *bias;
    tmp -= learning_rate * (*d_t);
    *bias = tmp;
    
    d_t.inc();
    bias.inc();
  End
}


/**
 * @brief Gradient with respect to previous hidden state and input
 *
 * Return value is a concatenation of dx_t (vector size hidden_size)
 * and dh_prev (vector size hidden_size).
 * These should be separated at the CPU side.
 *
 * Assumption: all matrices and vectors passed have same dimensions.
 *
 * @param forget_w Weight matrix forget get; input.
 * @param ret      result of the calculation 
 */
void gradient_previous_state_input(
  Int rows,
  Int columns,            // NOT blocks of 16
  Float::Ptr forget_W,
  Float::Ptr forget_d_t,
  Float::Ptr input_W,
  Float::Ptr input_d_t,
  Float::Ptr candidate_W,
  Float::Ptr candidate_d_t,
  Float::Ptr output_W,
  Float::Ptr output_d_t,
  Float::Ptr result
) {
  Int stride  = 4*columns;
  Int count   = columns >> 4;

  Float::Ptr f_d_t = forget_d_t;
  Float::Ptr i_d_t = input_d_t; 
  Float::Ptr c_d_t = candidate_d_t;
  Float::Ptr o_d_t = output_d_t;

  Float f_d_t_val = *f_d_t;
  Float i_d_t_val = *i_d_t; 
  Float c_d_t_val = *c_d_t;
  Float o_d_t_val = *o_d_t;

  For (Int i = 0, i < rows, i++)
    Int index  = (i & 0xf);
    Int offset = i*stride;

    Float::Ptr f_W = forget_W.offset(offset);
    Float::Ptr i_W = input_W.offset(offset);
    Float::Ptr c_W = candidate_W.offset(offset);
    Float::Ptr o_W = output_W.offset(offset);

    If (i > 0 && (index == 0))
      f_d_t.inc();
      i_d_t.inc();
      c_d_t.inc();
      o_d_t.inc();

      f_d_t_val = *f_d_t;
      i_d_t_val = *i_d_t; 
      c_d_t_val = *c_d_t;
      o_d_t_val = *o_d_t;
    End

    Float f_d_t_i;
    Float i_d_t_i;
    Float c_d_t_i;
    Float o_d_t_i;
    set_all(f_d_t_val, f_d_t_i, index);
    set_all(i_d_t_val, i_d_t_i, index);
    set_all(c_d_t_val, c_d_t_i, index);
    set_all(o_d_t_val, o_d_t_i, index);

    Float::Ptr res = result;
    For (Int j = 0, j < count, j++)
      Float tmp = *res;

      tmp += ((*f_W) * f_d_t_i) +
             ((*i_W) * i_d_t_i) + 
             ((*c_W) * c_d_t_i) +
             ((*o_W) * o_d_t_i);

      *res = tmp;

      res.inc();
      f_W.inc();
      i_W.inc();
      c_W.inc();
      o_W.inc();
    End
  End
}


/*
 * Previous calculation:
 *
 *    candidate.q_d_t = q_dc_next + q_dh_next.mul(output.q_activation.mul(q_c_t.tanh().dtanh()));
 */
void gradient_cell_state(
  Float::Ptr d_t,       // Output param
  Float::Ptr dc_next,
  Float::Ptr dh_next,
  Float::Ptr activation,
  Float::Ptr c_t,
  Int N
) {
  For (Int h = 0, h < N, h++)
    Float x = *c_t;
    Float x2 = tanh(x);
    Float x3 = 1.0f - x2*x2;     // dtanh()

    Float x4 = (*dc_next) + ((*dh_next) * (*activation) * x3);
    *d_t = x4;

    d_t.inc();
    dc_next.inc();
    dh_next.inc();
    activation.inc();
    c_t.inc();
  End
}


/*
 * Previous calculation:
 *
 *    input.q_d_t = candidate.q_d_t.mul(q_c_tilde.mul(input.q_activation.dsigmoid()));
 */
void gradient_input_gate(
  Float::Ptr input_d_t,       // Output param
  Float::Ptr candidate_d_t,
  Float::Ptr c_tilde,
  Float::Ptr activation,
  Int N
) {
  For (Int h = 0, h < N, h++)
    Float x  = *activation;
    Float x2 = x*(1.0f - x);  // dsigmoid()

    Float x3  = *c_tilde;

    Float x4 = (*candidate_d_t) * x3 * x2;
    *input_d_t = x4;

    input_d_t.inc();
    candidate_d_t.inc();
    c_tilde.inc();
    activation.inc();
  End
}


/*
 * Previous calculation:
 *
 *    output.q_d_t = q_dh_next.mul(q_c_t.tanh()).mul(output.q_activation.dsigmoid());
 */
void gradient_output_gate(
  Float::Ptr d_t,       // Output param
  Float::Ptr dh_next,
  Float::Ptr c_t,
  Float::Ptr activation,
  Int N,
  Float clip_value
) {
  Float clip_min = -1.0f*clip_value; // TODO: implement float operator-

  For (Int h = 0, h < N, h++)
    Float x  = *activation;
    Float x2 = x*(1.0f - x);  // dsigmoid()

    Float x3  = *c_t;
    Float x4  = tanh(x3);

    Float x5 = (*dh_next) * x4 * x2;
		kernel::clip_partial(x5, clip_min, clip_value);
    *d_t = x5;

    d_t.inc();
    dh_next.inc();
    c_t.inc();
    activation.inc();
  End
}


/*
 * Previous calculation:
 *
 *    candidate.q_d_t = candidate.q_d_t.mul(input.q_activation.mul(q_c_tilde.dtanh()));
 */
void gradient_candidate(
  Float::Ptr ret,       // Output param
  Float::Ptr d_t,
  Float::Ptr activation,
  Float::Ptr c_tilde,
  Int N,
  Float clip_value
) {
  Float clip_min = -1.0f*clip_value; // TODO: implement float operator-

  For (Int h = 0, h < N, h++)
    Float x  = *c_tilde;
    Float x2 = 1.0f - x*x;  // dtanh()

    Float x3 = (*d_t) * (*activation) * x2;

		kernel::clip_partial(x3, clip_min, clip_value);
    *ret = x3;

    ret.inc();
    d_t.inc();
    activation.inc();
    c_tilde.inc();
  End
}


/*
 * Previous calculation:
 *
 *    forget.q_d_t = candidate.q_d_t.mul(q_c_prev.mul(forget.q_activation.dsigmoid()));
 */
void gradient_forget(
  Float::Ptr ret,       // Output param
  Float::Ptr d_t,
  Float::Ptr c_prev,
  Float::Ptr activation,
  Int N,
  Float clip_value
) {
  Float clip_min = -1.0f*clip_value; // TODO: implement float operator-

  For (Int h = 0, h < N, h++)
    Float x  = *activation;
    Float x2 = x*(1.0f - x);  // dsigmoid()

    Float x3 = (*d_t) * (*c_prev) * x2;

		kernel::clip_partial(x3, clip_min, clip_value);
    *ret = x3;

    ret.inc();
    d_t.inc();
    c_prev.inc();
    activation.inc();
  End
}


/**
 * @brief Perform update state calculations for the forward step
 *
 * @param tmp  Vector for storing result of matrix*vector calculation
 * @param M    Width of matrix W and length of all vectors
 * @param N    Height of matrix W
 */
void forward_states(
  Float::Ptr x_h,
  Float::Ptr W,
  Float::Ptr tmp_ptr,           // Output param
  Float::Ptr q_b,
  Float::Ptr c_tilde,           // Output param
  Float::Ptr forget_activation,
  Float::Ptr c_prev,
  Float::Ptr input_activation,
  Float::Ptr c_t,               // Output param
  Float::Ptr output_activation,
  Float::Ptr h_t,               // Output param
  Int M,
  Int N
) {
  Int rows = M >> 4;
  Int count = N >> 4;

  // Following part of Cell state candidate
  Float::Ptr tmp_local = tmp_ptr;  // ptr changed in following calculation
	kernel::mult_vec_partial(x_h, W, tmp_local, rows, N);

  For (Int j = 0, j < count, j++)
    //
    // Cell state candidate
    // Original:     q_c_tilde = ((candidate.q_W*q_x_h) + candidate.q_b).tanh();
    // Calculation is exact.
    //
    Float x = *tmp_ptr;
    Float x2 = x + *q_b;
    Float tmp_c_tilde = tanh(x2);  // Interim value for c_tilde
    *c_tilde = tmp_c_tilde;
  
    //
    // Cell state update
    // Original: q_c_t = forget.q_activation.mul(q_c_prev) + input.q_activation.mul(q_c_tilde);
    //
    Float tmp_c_t = (*forget_activation) * (*c_prev) + (*input_activation) * tmp_c_tilde;
    *c_t = tmp_c_t;

    //
    // Hidden state
    // Original: auto q_h_t = output.q_activation.mul(q_c_t.tanh());
    //
    *h_t = (*output_activation) * tanh(tmp_c_t);


    tmp_ptr.inc();
    q_b.inc();
    c_tilde.inc();
    forget_activation.inc();
    c_prev.inc();
    input_activation.inc();
    c_t.inc();
    output_activation.inc();
    h_t.inc();
  End
}


// TODO: ptr's not cleaned up on exit, better would be shared or unique ptr.
BaseKernel *s_update_gate_kernel                   = nullptr;
BaseKernel *s_gradient_previous_state_input_kernel = nullptr;
BaseKernel *s_gradient_cell_state_kernel           = nullptr;
BaseKernel *s_gradient_input_gate_kernel           = nullptr;
BaseKernel *s_gradient_output_gate_kernel          = nullptr;
BaseKernel *s_gradient_candidate_kernel            = nullptr;
BaseKernel *s_gradient_forget_kernel               = nullptr;
BaseKernel *s_forward_states_kernel                = nullptr;

} // anon namespace


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


BaseKernel &gradient_cell_state_kernel() {
  if (s_gradient_cell_state_kernel == nullptr) {
    s_gradient_cell_state_kernel = new BaseKernel(
      compile(gradient_cell_state, settings()));
  }

  return *s_gradient_cell_state_kernel;
}


BaseKernel &gradient_input_gate_kernel() {
  if (s_gradient_input_gate_kernel == nullptr) {
    s_gradient_input_gate_kernel = new BaseKernel(
      compile(gradient_input_gate, settings()));
  }

  return *s_gradient_input_gate_kernel;
}


BaseKernel &gradient_output_gate_kernel() {
  if (s_gradient_output_gate_kernel == nullptr) {
    s_gradient_output_gate_kernel = new BaseKernel(
      compile(gradient_output_gate, settings()));
  }

  return *s_gradient_output_gate_kernel;
}


BaseKernel &gradient_candidate_kernel() {
  if (s_gradient_candidate_kernel == nullptr) {
    s_gradient_candidate_kernel = new BaseKernel(
      compile(gradient_candidate, settings()));
  }

  return *s_gradient_candidate_kernel;
}


BaseKernel &gradient_forget_kernel() {
  if (s_gradient_forget_kernel == nullptr) {
    s_gradient_forget_kernel = new BaseKernel(
      compile(gradient_forget, settings()));
  }

  return *s_gradient_forget_kernel;
}


BaseKernel &forward_states_kernel() {
  if (s_forward_states_kernel == nullptr) {
    s_forward_states_kernel = new BaseKernel(
      compile(forward_states, settings()));
  }

  return *s_forward_states_kernel;
}
