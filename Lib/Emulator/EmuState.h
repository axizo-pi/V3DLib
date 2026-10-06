#ifndef _V3DLIB_EMULATOR_EMUSTATE_H_
#define _V3DLIB_EMULATOR_EMUSTATE_H_
#include "EmuSupport.h"

namespace V3DLib {

class EmuState {
public:
  const bool run_v3d;
  int num_qpus;
  Word vpm[VPM_SIZE];      // Shared VPM memory

  EmuState(int in_num_qpus, IntList const &in_uniforms, bool in_run_v3d,  bool add_dummy);
  Vec get_uniform(int id, int &next_uniform);
  bool sema_inc(int sema_id);
  bool sema_dec(int sema_id);

  std::string dump_vpm() const;
  std::string dump_sema() const;

private:
  IntList uniforms;               // Kernel parameters
};

} // namespace V3DLib

#endif  //  _V3DLIB_EMULATOR_EMUSTATE_H_
