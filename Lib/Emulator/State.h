#ifndef _V3DLIB_EMULATOR_STATE_H_
#define _V3DLIB_EMULATOR_STATE_H_
#include "EmuState.h"
#include "QPUState.h"
#include "Common/SharedArray.h"  // Data

namespace V3DLib {

/**
 * @brief State of the VideoCore
 */
struct State : public EmuState {
  QPUState qpu[MAX_QPUS];  // State of each QPU
  Data emuHeap;

  State(int in_num_qpus, IntList const &in_uniforms) : EmuState(in_num_qpus, in_uniforms, true) {}
};


} // namespace V3DLib

#endif  // _V3DLIB_EMULATOR_STATE_H_
