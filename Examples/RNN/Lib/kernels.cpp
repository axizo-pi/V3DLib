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


/**
 * @brief Update gate weights and biases
 *
 * Dimension of `columns` in blocks of 16
 */
void update_gate(
	Float::Ptr W,
	Float::Ptr d_t,
	Float::Ptr x_h,
	Float::Ptr bias,
	Int rows,
	Int columns,
	Float learning_rate
) {
	Float tmp;
	Int stride = 16*4*columns;

  For (Int i = 0, i < rows, i++)
		Float::Ptr w_start = W.offset(i*stride);
		Float::Ptr x_h_start = x_h;

    For (Int j = 0, j < columns, j++)
			tmp = *w_start;
      tmp -= learning_rate * (*d_t) * (*x_h_start);
      *w_start = tmp;

			w_start.inc();
			x_h_start.inc();
    End

		tmp = *bias;
    tmp -= learning_rate * (*d_t);
		*bias = tmp;

		d_t.inc();
		bias.inc();
  End
}

