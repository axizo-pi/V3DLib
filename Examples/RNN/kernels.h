#ifndef _INCLUDE_RNN_KERNELS
#define _INCLUDE_RNN_KERNELS
#include "Source/Float.h"

using namespace V3DLib;

void kernel_sigmoid(Float::Ptr in, Float::Ptr bias, Float::Ptr out, Int N);
void kernel(Float::Ptr input, Float::Ptr mat, Float::Ptr result, Int M, Int N);
void outer_product(Float::Ptr left, Float::Ptr right, Float::Ptr out_matrix, Int N);
void vector_sub(Float::Ptr left, Float::Ptr right, Float::Ptr out, Int N);

#endif //  _INCLUDE_RNN_KERNELS
