#include "kernels.h"
#include "V3DLib.h"

namespace kernel {

/**
 * @brief calculate `sigmoid(in + bias)`.
 */
void sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *in;
    x += *bias;
    x = (1.0f/(1.0f + exp_e(-1.0f*x)));
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
void dsigmoid(Float::Ptr in, Float::Ptr out, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *in;
    x = x*(1.0f - x);
    *out = x;

    in.inc();
    out.inc();
  End
}


void tanh(Float::Ptr in, Float::Ptr out, Int N) {
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
 * Input is `in = tanh(x)`
 */
void dtanh(Float::Ptr in, Float::Ptr out, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *in;
    x = 1.0f - x*x;
    *out = x;

    in.inc();
    out.inc();
  End
}


void mul_element(Float::Ptr out, Float::Ptr lhs, Float::Ptr rhs, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = (*lhs) * (*rhs);
    *out = x;

    lhs.inc();
    rhs.inc();
    out.inc();
  End
}


void matrix_add(Float::Ptr ret, Float::Ptr lhs, Float::Ptr rhs, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = (*lhs) + (*rhs);
    *ret = x;

    ret.inc();
    lhs.inc();
    rhs.inc();
  End
}


void mul_float(Float::Ptr ret, Float::Ptr lhs, Float val, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = (*lhs) * val;
    *ret = x;

    ret.inc();
    lhs.inc();
  End
}


void matrix_add_self(Float::Ptr lhs, Float::Ptr rhs, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = (*lhs) + (*rhs);
    *lhs = x;

    lhs.inc();
    rhs.inc();
  End
}


void matrix_sub_self(Float::Ptr lhs, Float::Ptr rhs, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = (*lhs) - (*rhs);
    *lhs = x;

    lhs.inc();
    rhs.inc();
  End
}


/**
 * @brief Multiply matrix mat with vector input.
 *
 * Exposed for combined kernels.
 */
void mult_vec_partial(Float::Ptr &input, Float::Ptr &mat, Float::Ptr &result, Int &M, Int &N) {
  Float res = 0;
  Int mat_offset = 4*16*M;

  For (Int n = 0, n < N, n++)
    If (n > 0 && ((n & 0xf) == 0))
      *result = res;
      result.inc();
      res = 0.0f;
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

    Where (index() == (n & 0xf))
      res = tmp;
    End

  End

  *result = res;
}


/**
 * @brief Multiply matrix mat with vector input.
 *
 * The matrix calculation is:
 *
 *     result = mat*input
 *
 * @param input   vector to multiply with
 * @param mat     matrix to multiply with
 * @param result  array to store result in
 * @param M       length of vector in blocks of 16
 * @param N       height of matrix
 */
void mult_vec(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N) {
  mult_vec_partial(input, mat, result, M, N);
}


/**
 * @brief Multiply matrix mat with vector input, where matrix is transposed in the calculation.
 *
 * The matrix calculation is:
 *
 *     result = mat^T*input
 *
 * @param input   vector to multiply with
 * @param mat     matrix to multiply with, not transposed beforehand
 * @param result  array to store result in
 * @param M       length of vector
 * @param N       height of matrix, in blocks of 16
 */
void mult_vec_transposed(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N) {
  Float res = 0.0f;
  Float tmp = 0.0f;
  Float::Ptr mat_base = mat;
  mat_base -= index();

  For (Int m = 0, m < M, m++)     // Iterate over matrix columns
    If (m > 0 && ((m & 0xf) == 0))
      *result = res;
      result.inc();
      res = 0.0f;
    End

    Float::Ptr vec = input;

    For (Int n = 0, n < N, n++)   // Iterate over matrix rows
      Float row  = *(mat_base + m + n*16*M + M*index());

      tmp += (*vec)*row;
      vec.inc();
    End

    rotate_sum(tmp, tmp);
    res.set_at(m, tmp);
    tmp = 0.0f;
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
		Float left_val = *left;
    Float::Ptr start = right;

    For (Int j = 0, j < M, j++)
      *out_matrix = left_val * *start;

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
 * Exposed for combined kernels
 */
void clip_partial(Float &val, Float &clip_min, Float &clip_max) {
  Where (val > clip_max)
    val = clip_max;
  End

  Where (val < clip_min)
    val = clip_min;
  End
}


/**
 * `clip_value` assumed to be positive
 */
void clip(Float::Ptr in, Float::Ptr out, Int N, Float clip_value) {
  Float tmp;
  Float clip_min = -1.0f*clip_value; // TODO: implement float operator-

  For (Int i = 0, i < N, i++)
    tmp = *in; 
    clip_partial(tmp, clip_min, clip_value);
    *out = tmp;

    in.inc();
    out.inc();
  End
}

} // namespace kernel
