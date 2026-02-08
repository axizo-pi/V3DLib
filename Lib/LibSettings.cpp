#include "LibSettings.h"
#include "Support/basics.h"

namespace V3DLib {
namespace {

int const DEFAULT_HEAP_SIZE = 8*1024*1024;
int const QPU_TIMEOUT = 10;

int  _heap_size                 = -1;     // bytes, size of shared (CPU-GPU) memory
int  _qpu_timeout               = -1;     // seconds, time to wait for response from QPU
bool _use_tmu_for_load          = true;   // vc4 only, ignored for v3d. If false, use DMA
bool _use_high_precision_sincos = false;  // If true, add extra precision to sin/cos calculation for function version
bool _dump_line_numbers         = true;  // output line numbers for dump()

}  // anon namespace

namespace LibSettings {

/**
 * Get qpu timeout in seconds
 */
int qpu_timeout() {
  if (_qpu_timeout == -1) {
    _qpu_timeout = QPU_TIMEOUT;
  }

  return _qpu_timeout;
}


void qpu_timeout(int val) {
  assert(val > 0);
  assert(_qpu_timeout == -1); // For now, allow setting it only once
  _qpu_timeout = val;
}


/**
 * Get heap size in bytes
 */
int heap_size() {
  if (_heap_size == -1) {
    _heap_size = DEFAULT_HEAP_SIZE;
  }

  return _heap_size;
}


/**
 * Set heap size
 *
 * @param size_in_bytes  size of heap in bytes
 */
void heap_size(int val) {
  assert(val > 0);                  // also to guard against overflow
  _heap_size = val;
}


bool use_tmu_for_load()         { return _use_tmu_for_load; }
void use_tmu_for_load(bool val) { _use_tmu_for_load = val; }


bool use_high_precision_sincos()         { return _use_high_precision_sincos; }
void use_high_precision_sincos(bool val) { _use_high_precision_sincos = val; }

bool dump_line_numbers() { return _dump_line_numbers; }

} // namespace LibSettings
}  // namespace V3DLib
