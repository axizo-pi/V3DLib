#ifndef _GRU_KERNEL_H
#define _GRU_KERNEL_H
#include "Kernel.h"
#include "matrix.h"

namespace gru_kernel {

void back_prop_1(qpu::matrix &ret, qpu::matrix const &ds_cur, qpu::matrix const &q_z, qpu::matrix const &q_h);

} // namespace gru_kernel

#endif //  _GRU_KERNEL_H
