#ifndef _GRU_KERNEL_H
#define _GRU_KERNEL_H
#include "Kernel.h"
#include "matrix.h"

namespace gru_kernel {

void back_prop_1(qpu::matrix &ret, qpu::matrix const &ds_cur, qpu::matrix const &q_z, qpu::matrix const &q_h);
void back_prop_3(qpu::matrix &ret, qpu::matrix const &dsr, qpu::matrix const &S, qpu::matrix const &r);
void back_prop_4(qpu::matrix &ret, qpu::matrix const &ds_cur_bk, qpu::matrix const &z, qpu::matrix const &S, qpu::matrix const &h);
void set_decay(qpu::matrix &lhs, qpu::matrix const &rhs, float decay);
void divide_matrix(qpu::matrix &ret, qpu::matrix &lhs, qpu::matrix const &rhs);

} // namespace gru_kernel

#endif //  _GRU_KERNEL_H
