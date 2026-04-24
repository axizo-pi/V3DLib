#ifndef _LSTM_LSTM_CONVERT_H
#define _LSTM_LSTM_CONVERT_H
#include "matrix.h"
#include "lstm_matrix.h"

qpu::vector copy(lstm::vector const &rhs);
qpu::matrix copy(lstm::matrix const &rhs);
bool same(qpu::vector const &lhs, lstm::vector const &rhs, float precision = 0.0f);
bool same(qpu::matrix const &lhs, lstm::matrix const &rhs);
qpu::vector qpu_concat(lstm::vector const &x, lstm::vector const &y);

#endif // _LSTM_LSTM_CONVERT_H
