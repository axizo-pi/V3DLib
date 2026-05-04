#ifndef _INCLUDE_RNN_TEST
#define _INCLUDE_RNN_TEST
#include "BaseKernel.h"

void test_outer_product(V3DLib::BaseKernel &op, bool do_kernel);
void test_vector();
void test_back_propagation(V3DLib::BaseKernel &op);

#endif //  _INCLUDE_RNN_TEST
