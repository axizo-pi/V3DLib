#ifndef _GRU_FORWARD_H
#define _GRU_FORWARD_H
#include "model.h"

void forward_propagation(
  Model &m,
  MMatrix const &X,
  MMatrix const &Y,  // Not used in test
  State &state,
  int time_steps,
  int input_dim,
  int hidden_dim,
  int output_dim,
  bool do_test = false
);

#endif // _GRU_FORWARD_H
