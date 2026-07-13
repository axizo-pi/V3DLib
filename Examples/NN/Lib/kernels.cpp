#include "kernels.h"
#include "V3DLib.h"

namespace kernel {

void max_partial(Float::Ptr &rhs, Float &result, Int &col_size) {
  Float acc = -1024.0f;  // Dummy init value, chosen very low

   For (Int c = 0, c < col_size, c++)
    Float tmp = *rhs;
    acc = V3DLib::max(acc, tmp);

    rhs.inc();
  End

  rotate_max(acc, result);
}


void softmax_partial(Float::Ptr &lhs, Float &max, Int &col_size) {
  For (Int c = 0, c < col_size, c++)
    Float tmp = *lhs;
    Float tmp2 = V3DLib::exp_e(tmp - max);

    *lhs = tmp2;
    lhs.inc();
  End
}  


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
    x = V3DLib::tanh(x);
    *out = x;

    in.inc();
    out.inc();
  End
}


void ln(Float::Ptr in, Float::Ptr out, Int N) {
  For (Int h = 0, h < N, h++)
    Float x = *in;
    x = V3DLib::ln(x);
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


void matrix_sub(Float::Ptr ret, Float::Ptr lhs, Float::Ptr rhs, Int N) {
  //TODO: For (Int h = me(), h < N, h += numQPUs()) - init Ptr's
  For (Int h = 0, h < N, h++)
    Float x = (*lhs) - (*rhs);
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

/**
 * Tested for 0 < num QPU's <= 8 and max QPU's
 */
void mul_float_self(Float::Ptr lhs, Float val, Int N) {
  // TODO: check if correct. Expecting Ptr to be initialized for multi-QPU
	Int offset = 16*numQPUs();
	lhs += 16*me();

  For (Int h = me(), h < N, h += numQPUs())
    Float x = (*lhs) * val;
    *lhs = x;

		lhs += offset;
  End

/*
	// Works as expected
  For (Int h = 0, h < N, h++)
    Float x = (*lhs) * val;
    *lhs = x;

    lhs.inc();
  End
*/	
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
 * In GRU test mostly lhs_rows == 1; doing multi-QPU over rows is useless
 */
void mult_matrix(Float::Ptr in_ret, Float::Ptr lhs, Float::Ptr rhs, Int lhs_rows, Int inner, Int rhs_cols) {
  Float::Ptr rhs_base = rhs;
  rhs_base           -= index();
  Int rhs_offset      = index()*rhs_cols;
  rhs_base           += rhs_offset;

  Int block_size = inner >> 4;
  Int rhs_inc    = 16*rhs_cols;
  Int ret_cols   = ((rhs_cols + 15) & ~0xf);   // Output width must be a multiple of 16

  Float ret_acc = 0.0f;
  Float::Ptr ret;

  For (Int row = me(), row < lhs_rows, row += numQPUs())
    ret = in_ret + row*ret_cols;

    For (Int col = 0, col < rhs_cols, col++)
      If (col > 0 && (col & 0xf) == 0)
        *ret = ret_acc;
        ret.inc();
        ret_acc = 0;
      End

      Float::Ptr lhs_row = lhs + row*inner;
      Float::Ptr rhs_col = rhs_base + col;
      Float acc = 0.0f;

      For (Int block = 0, block < block_size, block++)
        acc += *lhs_row * *rhs_col;

        lhs_row.inc();
        rhs_col += rhs_inc;
      End

      rotate_sum(acc, acc);

      Where (index() == (col & 0xf))
        ret_acc = acc;
      End
    End

    *ret = ret_acc;
    ret_acc = 0;
  End
}


/**
 * @brief Column-first matrix multiplication.
 *
 * The outer loop goes over the width of the rhs.
 *
 * This is less efficient than row-first, but can better utilize multi-QPU's.
 */
void mult_matrix_col(Float::Ptr ret, Float::Ptr lhs, Float::Ptr rhs, Int lhs_rows, Int inner, Int rhs_cols) {
  Float::Ptr ret_base = ret;
  ret_base           -= index();

  Int rhs_offset      = index()*rhs_cols;
  Int rhs_inc         = 16*rhs_cols;

  Float::Ptr rhs_base = rhs;
  rhs_base           -= index();
  rhs_base           += rhs_offset;

  Int block_size = inner >> 4;

  For (Int col = me(), col < rhs_cols, col += numQPUs())
    Float::Ptr rhs_col = rhs_base + col;

    For (Int row = 0, row < lhs_rows, row++)
      Float::Ptr lhs_row = (lhs + (row*inner));  comment("Init lhs row");
      Float acc = 0.0f;

      For (Int block = 0, block < block_size, block++)
        acc += *lhs_row * *rhs_col;  comment("increment acc");

        lhs_row.inc();
        rhs_col += rhs_inc;
      End

      rotate_sum(acc, acc);
      *(ret_base + row*rhs_cols + col) = acc;
    End
  End
}


/**
 * @brief Multiplication of two matrices, where the rhs is regarded as transposed
 */
void mult_matrix_t(Float::Ptr in_ret, Float::Ptr lhs, Float::Ptr rhs, Int lhs_rows, Int inner, Int rhs_rows) {
  Float ret_acc = 0.0f;
  Float::Ptr ret;

  For (Int l_row = me(), l_row < lhs_rows, l_row += numQPUs())
    ret = in_ret + l_row*((rhs_rows + 15) & ~0xf);  // output width must be a multiple of 16

    For (Int r_row = 0, r_row < rhs_rows, r_row++)
      If (r_row > 0 && (r_row & 0xf) == 0)
        *ret = ret_acc;
        ret.inc();
        ret_acc = 0;
      End

      Float::Ptr lhs_row = lhs + l_row*inner;
      Float::Ptr rhs_row = rhs + r_row*inner;
      Float acc = 0.0f;

      For (Int block = 0, block < (inner >> 4), block++)
        acc += *lhs_row * *rhs_row;

        lhs_row.inc();
        rhs_row.inc();
      End

      rotate_sum(acc, acc);

      Where (index() == (r_row & 0xf))
        ret_acc = acc;
      End
    End

    *ret = ret_acc;
    ret_acc = 0;
  End
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
void outer_product(Float::Ptr ret, Float::Ptr left, Float::Ptr right, Int N, Int M) {
  left -= index();

  For (Int i = 0, i < N, i++)
    Float left_val = *left;
    Float::Ptr start = right;

    For (Int j = 0, j < M, j++)
      *ret = left_val * *start;

      ret.inc();
      start.inc();
    End

    left++;
  End
}


namespace {

/**
 * NumQPU's:
 *
 * - 1,2,4: Works perfectly
 * - 6    : Fails train loop 1
 * - 8    : Works sometimes within precision OR fails train loop 1,3
 * - 10   : Fails train loop 0
 * - 12   : Fails train loop 0
 * - 16   : Fails train loop 1
 *
 * TODO: Research this again, expectation is that it should always work
 */
void outer_add_partial(Float::Ptr &ret, Float::Ptr &lhs, Float::Ptr &rhs, Int &N, Int &M) {
  lhs -= index();  comment("Start outer_add_partial");

	Int Start  = me();
	Int Inc    = numQPUs();
	Int Offset = M << 4;

  For (Int i = Start, i < N, i += Inc)
    Float lhs_val        = *(lhs + i);
    Float::Ptr ret_start = ret + i*Offset;
    Float::Ptr rhs_start = rhs;

    For (Int j = 0, j < M, j++)
      // The only logical difference with outer_product() is `*res_start += ...`
      Float val = *ret_start + (*rhs_start * lhs_val);
      *ret_start = val;

      ret_start.inc();
      rhs_start.inc();
    End
  End
}

}  // anon namespace


void outer_add(Float::Ptr ret, Float::Ptr left, Float::Ptr right, Int N, Int M) {
  outer_add_partial(ret, left, right, N, M);
}


void outer_add_rows(Float::Ptr ret, Float::Ptr lhs, Float::Ptr rhs, Int lhs_rows, Int lhs_cols, Int rhs_cols) {
  Int rhs_step = rhs_cols >> 4;

  For (Int row = 0, row < lhs_rows, row++)
    Float::Ptr start_ret = ret;

    Int lhs_row = row*lhs_cols;
    Int rhs_row = row*rhs_cols;

    Float::Ptr lhs_offset = lhs + lhs_row;  comment("Set lhs_offset");
    Float::Ptr rhs_offset = rhs + rhs_row;

    outer_add_partial(start_ret, lhs_offset, rhs_offset, lhs_cols, rhs_step);
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
 * @brief Calculate max per row
 */
void max_row(Float::Ptr ret, Float::Ptr in_rhs, Int rows, Int cols) {
  ret -= index();
  Int col_size = cols >> 4;

  For (Int r = 0, r < rows, r++)
    Float::Ptr rhs = in_rhs + r*cols;
    Float tmp2;

    max_partial(rhs, tmp2, col_size);

    *ret = tmp2;
    ret++;
  End
}


void softmax(Float::Ptr lhs, Float max, Int cols) {
  Int col_size = cols >> 4;
  softmax_partial(lhs, max, col_size);
}


/**
 * @brief Calculate softmax per row
 *
 * Changes are stored inline in lhs
 */
void softmax_rows(Float::Ptr in_lhs, Float::Ptr in_max, Int rows, Int cols) {
  in_max -= index();
  Int col_size = cols >> 4;

  For (Int r = 0, r < rows, r++)
    Float::Ptr lhs = in_lhs + r*cols;
    Float max = *in_max;

    softmax_partial(lhs, max, col_size);

    in_max++;
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
