#include "kernels.h"
#include "V3DLib.h"

using namespace V3DLib;


void kernel_sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out, Int N) {
  Int h;
  For (h = 0, h < N, h++)
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
 * Multiply matrix mat with vector input.
 *
 * @param M length of vector in blocks of 16
 * @param N height of matrix.
 */
void kernel_mult_vec(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N) {
  Float res = 0;
  Int mat_offset = 4*16*M;

   For (Int n = 0, n < N, n++)
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

    If (n > 0 && n % 16 == 0)
      *result = res;
      result.inc();
      res = 0;
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
