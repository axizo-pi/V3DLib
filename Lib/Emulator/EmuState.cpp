#include "EmuState.h"

namespace V3DLib {
namespace {
  //
  // Semaphores are shared over the QPU's
  //
  int const NUM_SEMAPHORES = 16;
  int sema[NUM_SEMAPHORES];       // Semaphores

  // Protection against locks due to semaphore waiting
  int const MAX_SEMAPHORE_WAIT = 48*1024;   // See comment in sema_inc()
  int semaphore_wait_count = 0;

} // anon namespace


EmuState::EmuState(int in_num_qpus, IntList const &in_uniforms, bool in_run_v3d, bool add_dummy) :
  run_v3d(in_run_v3d),
  num_qpus(in_num_qpus),
  uniforms(in_uniforms)
{
  // Initialise semaphores
  for (int i = 0; i < NUM_SEMAPHORES; i++) sema[i] = 0;

  if (add_dummy) {
    // Add final dummy uniform for emulator
    // See Note 1, function `invoke()` in `vc4/Invoke.cpp`.
    uniforms << 0;
  }
}

/**
 * Uniforms vc4:
 *   - me()
 *   - num QPU's
 *   - uniform values
 *   - dummy
 *
 * Uniforms v3d:
 *   - me()
 *   - num QPU's
 *   - devnull
 *   - uniform values
 */ 
Vec EmuState::get_uniform(int id, int &next_uniform) {
  //warn << "next_uniform: " << next_uniform << ", uniforms.size(): " << uniforms.size();
  assert(next_uniform < uniforms.size());

  Vec a;

  if (run_v3d) {
    switch(next_uniform) {
    case -3:
      a = id;
      break;
    case -2:
      a = num_qpus;
      break;
    case -1:
      // Dummy
      break;
    default:
      a = uniforms[next_uniform];
      break;
    }
  } else {
    switch(next_uniform) {
    case -2:
      a = id;
      break;
    case -1:
      a = num_qpus;
      break;
    default:
      a = uniforms[next_uniform];
      break;
    }
  }

  next_uniform++;
  return a;
}


/**
 * @brief Increment semaphore
 *
 * @return true if QPU stalls on semaphore, false if can continue
 */
bool EmuState::sema_inc(int sema_id) {
  assert(sema_id >= 0 && sema_id < 16);
  if (sema[sema_id] == 15) {
    semaphore_wait_count++;

    //
    // Following fails with high wait count values, for e.g.:
    //
    //     sudo ./obj/qpu-debug/bin/Mandelbrot -n=4 -grey -r=debugger -dim=128
    //
    // - max wait count 31323
    //
    // QPU run works perfectly fine; this is probably a wrong assumption.
    //
    assertq(semaphore_wait_count < MAX_SEMAPHORE_WAIT, "Semaphore wait for SINC appears to be stuck");
    return true;
  } else {
    semaphore_wait_count = 0;
    sema[sema_id]++;
    return false;
  }
}


/**
 * @brief Decrement semaphore
 *
 * @return true if QPU stalls on semaphore, false if can continue
 */
bool EmuState::sema_dec(int sema_id) {
  assert(sema_id >= 0 && sema_id < 16);
  if (sema[sema_id] == 0) {
    semaphore_wait_count++;

    // See comment in sema_inc()
    assertq(semaphore_wait_count < MAX_SEMAPHORE_WAIT, "Semaphore wait for SINC appears to be stuck");
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

    if (last_val == 0) {
      ret << "(" << last_val << str_count << ") ";
    } else {
      ret << last_index << ":" << last_val << str_count << ", ";
    }

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


std::string EmuState::dump_sema() const {
  std::string ret;

  ret << "semaphores: ";

  for (int i = 0; i < NUM_SEMAPHORES; ++i) {
    ret << sema[i] << " ";
  }

  ret << "; wait: " << semaphore_wait_count << "\n";

  return ret;
}

} // namespace V3DLib
