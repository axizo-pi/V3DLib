#include "kernels.h"
#include "V3DLib.h"

using namespace V3DLib;

/**
 * @brief calculate `sigmoid(in + bias)`.
 */
void kernel_sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *in;

    x += *bias;
    x = (1.0/(1.0 + exp_e(-1.0*x)));

    *out = x;

    in.inc();
    bias.inc();
    out.inc();
  End
}


/**
 * @brief Calculate the derivate of `sigmoid()`.
 *
 * Input is `v = sigmoid(x)`
 */
void kernel_dsigmoid(Float::Ptr in, Float::Ptr out, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *in;

    x = x*(1.0 - x);

    *out = x;

    in.inc();
    out.inc();
  End
}


void kernel_tanh(Float::Ptr in, Float::Ptr out, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *in;

    x = tanh(x);

    *out = x;

    in.inc();
    out.inc();
  End
}


/**
 * @brief Calculate the derivate of `tanh()`.
 *
 * Input is `v = tanh(x)`
 */
void kernel_dtanh(Float::Ptr in, Float::Ptr out, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *in;

    x = 1.0f - x*x;

    *out = x;

    in.inc();
    out.inc();
  End
}


/**
 * Multiply matrix mat with vector input.
 *
 * @param M length of vector in blocks of 16
 * @param N height of matrix.
 */
void kernel_mult_vec(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N) {
  Float res = 0;
  Int mat_offset = 4*16*M;

   For (Int n = 0, n < N, n++)
    If (n > 0 && (n % 16 == 0))
      *result = res;
      result.inc();
      res = 0;
    End

    Float tmp = 0.0f;

    Float::Ptr tmp_mat = mat.offset(n*mat_offset);
    Float::Ptr tmp_vec = input;

     For (Int m = 0, m < M, m++)
      tmp += (*tmp_mat) * (*tmp_vec);
      tmp_mat.inc();
      tmp_vec.inc();
    End

    rotate_sum(tmp, tmp);

    Where (index() == (n % 16))
      res = tmp;
    End

  End

  *result = res;
}


/**
 * @param N  length of left input vector
 * @param M  length of right input vector, in blocks of 16
 */
void outer_product(Float::Ptr left, Float::Ptr right, Float::Ptr out_matrix, Int N, Int M) {
  left -= index();

  For (Int i = 0, i < N, i++)
    Float::Ptr start = right;

    For (Int j = 0, j < M, j++)
      *out_matrix = *left * *start;

      out_matrix.inc();
      start.inc();
    End

    left++;
  End
}


void vector_sub(Float::Ptr left, Float::Ptr right, Float::Ptr out, Int N) {
  Float tmp;

  For (Int i = 0, i < N, i++)
    tmp = *left - *right; 
    *out = tmp;

    left.inc();
    right.inc();
    out.inc();
  End
}


void vector_add(Float::Ptr left, Float::Ptr right, Float::Ptr out, Int N) {
  Float tmp;

  For (Int i = 0, i < N, i++)
    tmp = *left + *right; 
    *out = tmp;

    left.inc();
    right.inc();
    out.inc();
  End
}


/**
 * `clip_value` assumed to be positive
 */
void kernel_clip(Float::Ptr in, Float::Ptr out, Int N, Float clip_value) {
  Float tmp;
  Float clip_min = -1.0f*clip_value; // TODO: implement float operator-

  For (Int i = 0, i < N, i++)
    tmp = *in; 

    Where (tmp > clip_value)
      tmp = clip_value;
    End

    Where (tmp < clip_min)
      tmp = clip_min;
    End

    *out = tmp;
    in.inc();
    out.inc();
  End
}

namespace {

/**
 * @brief Set all values in vector to value at index n
 */
void set_all(Float &input, Float &result, Int n) {
  Float tmp = 0.0f;
  set_at(tmp, n, input);
  rotate_sum(tmp, result);
}

} // anon namespace


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
