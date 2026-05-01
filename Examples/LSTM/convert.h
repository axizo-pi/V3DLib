#ifndef _LSTM_LSTM_CONVERT_H
#define _LSTM_LSTM_CONVERT_H
#include "matrix.h"
#include "lstm_matrix.h"

int resize(std::size_t in_size, bool do_dump = false);
lstm::vector copy(qpu::vector const &rhs, int offset = 0, int length = -1);
qpu::vector copy(lstm::vector const &rhs);
qpu::matrix copy(lstm::matrix const &rhs);
bool same(qpu::vector const &lhs, lstm::vector const &rhs, float precision = 0.0f);
bool same(qpu::matrix const &lhs, lstm::matrix const &rhs, float precision = 0.0f);
bool same(qpu::vector const &lhs, qpu::vector const &rhs, float precision = 0.0f);
bool same(lstm::vector const &lhs, lstm::vector const &rhs, float precision = 0.0f);
qpu::vector qpu_concat(lstm::vector const &x, lstm::vector const &y);

#endif // _LSTM_LSTM_CONVERT_H
