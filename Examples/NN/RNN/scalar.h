#ifndef _INCLUDE_RNN_SCALAR
#define _INCLUDE_RNN_SCALAR
#include "Source/Float.h"

using namespace V3DLib;

namespace scalar {

void  sigmoid(Float::Array &vec, Float::Array const &bias, Float::Array &output);
void  mult(Float::Array const &vec, Float::Array const &mat, Float::Array &res);
float loss(Float::Array const &result, Float::Array const &y);

} // namespace scalar

#endif // _INCLUDE_RNN_SCALAR
