#include "EmuState.h"
#include "Support/basics.h"

namespace V3DLib {

Vec const EmuState::index_vec({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15});


EmuState::EmuState(int in_num_qpus, IntList const &in_uniforms, bool add_dummy) :
  num_qpus(in_num_qpus),
  uniforms(in_uniforms)
{
  // Initialise semaphores
  for (int i = 0; i < 16; i++) sema[i] = 0;

  if (add_dummy) {
    // Add final dummy uniform for emulator
    // See Note 1, function `invoke()` in `vc4/Invoke.cpp`.
    uniforms << 0;
  }
}


Vec EmuState::get_uniform(int id, int &next_uniform) {
  Vec a;

  assert(next_uniform < uniforms.size());
  if (next_uniform == -2)
    a = id;
  else if (next_uniform == -1)
    a = num_qpus;
  else
    a = uniforms[next_uniform];

  next_uniform++;
  return a;
}


/**
 * Increment semaphore
 */
bool EmuState::sema_inc(int sema_id) {
  assert(sema_id >= 0 && sema_id < 16);
  if (sema[sema_id] == 15) {
    semaphore_wait_count++;
    assertq(semaphore_wait_count < MAX_SEMAPHORE_WAIT, "Semaphore wait for SINC appears to be stuck");
    return true;
  } else {
    semaphore_wait_count = 0;
    sema[sema_id]++;
    return false;
  }
}


/**
 * Decrement semaphore
 */
bool EmuState::sema_dec(int sema_id) {
  assert(sema_id >= 0 && sema_id < 16);
  if (sema[sema_id] == 0) {
    semaphore_wait_count++;
    assertq(semaphore_wait_count < MAX_SEMAPHORE_WAIT, "Semaphore wait for SDEC appears to be stuck");
    return true;
  } else {
    semaphore_wait_count = 0;
    sema[sema_id]--;
    return false;
  }
}


std::string EmuState::dump_vpm() const {
  std::string ret;

  ret << "vpm:\n  ";

  int last_index   = -1;
  int last_count   =  0;
  int32_t last_val =  0;

  auto disp = [&] () -> std::string {
    std::string ret;

    // Showing int only (for now)
    std::string str_count;
    if (last_count > 1) {
      str_count << "," << last_count << "x";
    }

    ret << last_index << ":" << last_val << str_count << ", ";
    return ret;
  };


  for (int i = 0; i < VPM_SIZE; i++) {
    if (last_index == -1) {
      last_index = i;
      last_count = 1;
      last_val   = vpm[i].intVal;
      continue;
    } else if (vpm[i].intVal == last_val) {
      last_count++;
      continue;
    }

    ret << disp();

    last_index = i;
    last_count = 1;
    last_val   = vpm[i].intVal;
  }

  ret << disp() << "\n";
  return ret;
}

} // namespace V3DLib
