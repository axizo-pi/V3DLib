#ifndef _INCLUDE_RNN_RNN_DATA
#define _INCLUDE_RNN_RNN_DATA
#include <vector>

namespace rnn {

const int NumInputs  = 3;

typedef std::vector<std::vector<float>> Inputs;  // TODO: Misnomer, also used for expected values

Inputs inputs();
Inputs expected();

}  // namespace rnn

#endif  //  _INCLUDE_RNN_RNN_DATA
