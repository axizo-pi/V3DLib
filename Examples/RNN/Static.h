#ifndef _INCLUDE_RNN_STATIC
#define _INCLUDE_RNN_STATIC
#include "Source/Float.h"

using namespace V3DLib;

void  frand_array(Float::Array &rhs);
void  scalar_sigmoid(Float::Array &vec, Float::Array const &bias, Float::Array &output);
void  run_scalar(Float::Array const &vec, Float::Array const &mat, Float::Array &res);
float loss(Float::Array const &result, Float::Array const &y);

#endif // _INCLUDE_RNN_STATIC
