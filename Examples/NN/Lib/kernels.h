#ifndef _INCLUDE_RNN_KERNELS
#define _INCLUDE_RNN_KERNELS
#include "Kernel.h"
#include "Source/Float.h"

namespace kernel {

using namespace V3DLib;

void mult_vec_partial(Float::Ptr &input, Float::Ptr &mat, Float::Ptr &result, Int &M, Int &N);
void clip_partial(Float &val, Float &clip_min, Float &clip_max);

void sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out, Int N);
void dsigmoid(Float::Ptr in, Float::Ptr out, Int N);
void tanh(Float::Ptr in, Float::Ptr out, Int N);
void dtanh(Float::Ptr in, Float::Ptr out, Int N);
void mul_element(Float::Ptr out, Float::Ptr lhs, Float::Ptr rhs, Int N);
void matrix_add(Float::Ptr ret, Float::Ptr lhs, Float::Ptr rhs, Int N);
void matrix_add_self(Float::Ptr lhs, Float::Ptr rhs, Int N);
void mult_vec(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N);
void mult_vec_transposed(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N);
void outer_product(Float::Ptr left, Float::Ptr right, Float::Ptr out_matrix, Int N, Int M);
void vector_sub(Float::Ptr left, Float::Ptr right, Float::Ptr out, Int N);
void vector_add(Float::Ptr left, Float::Ptr right, Float::Ptr out, Int N);
void clip(Float::Ptr in, Float::Ptr out, Int N, Float clip_value);

} // namespace kernel

#endif //  _INCLUDE_RNN_KERNELS
