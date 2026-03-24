#ifndef _V3DLIB_EMULATOR_EMUSTATE_H_
#define _V3DLIB_EMULATOR_EMUSTATE_H_
#include "EmuSupport.h"

namespace V3DLib {

class EmuState {
public:
  int num_qpus;
  Word vpm[VPM_SIZE];      // Shared VPM memory

  EmuState(int in_num_qpus, IntList const &in_uniforms, bool add_dummy = false);
  Vec get_uniform(int id, int &next_uniform);
  bool sema_inc(int sema_id);
  bool sema_dec(int sema_id);

  std::string dump_vpm() const;

  static Vec const index_vec;

private:
  IntList uniforms;        // Kernel parameters
  int sema[16];            // Semaphores

  // Protection against locks due to semaphore waiting
  int const MAX_SEMAPHORE_WAIT = 1024;
  int semaphore_wait_count = 0;
};

} // namespace V3DLib

#endif  //  _V3DLIB_EMULATOR_EMUSTATE_H_
