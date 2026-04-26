#ifndef _INCLUDE_RNN_KERNELS
#define _INCLUDE_RNN_KERNELS
#include "Kernel.h"
#include "Source/Float.h"

using namespace V3DLib;

void kernel_sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out, Int N);
void kernel_dsigmoid(Float::Ptr in, Float::Ptr out, Int N);
void kernel_tanh(Float::Ptr in, Float::Ptr out, Int N);
void kernel_dtanh(Float::Ptr in, Float::Ptr out, Int N);
void kernel_mult_vec(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N);
void outer_product(Float::Ptr left, Float::Ptr right, Float::Ptr out_matrix, Int N, Int M);
void vector_sub(Float::Ptr left, Float::Ptr right, Float::Ptr out, Int N);
void vector_add(Float::Ptr left, Float::Ptr right, Float::Ptr out, Int N);

#endif //  _INCLUDE_RNN_KERNELS
