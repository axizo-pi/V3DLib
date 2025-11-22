#ifdef QPU_MODE

// 
// Converted from: https://github.com/Idein/py-videocore6/blob/ec275f668f8aa4c89839fb8095b74f402260b1a6/videocore6/driver.py
//
// Note 1:
//
// TODO all the following in this is not wrong due to code issues, redo
//
// Setting workgroup value to non-zero has the effect that:
//
//  - Some QPU's don't return anything, which QPU's varies
//  - Other QPU's return 1 or a limited number of registers, which QPU's varies
//  - The reg's that *are* returned, have the correct value
//
// special observed cases:
//
// (1,7,1)     0
// (1,1,1)     0
// (2,2,1)     0
//  - consistently 1 qpu with only reg 0 filled, rest filled
//
// 1,1,2     0
//  - consistently one QPU with first 4 reg's filled, which QPU varies
//
// 1,1,1     1
//  - 4-6 QPU's return with only reg 0 filled, which QPU's vary
//
// 1,1,2     1
//  - 5-6 QPU's return with only reg 0 and 1 filled, which QPU's vary
//
// 1,1,7     1
//  - >= 4 QPU's return with only reg's 0-6 filled, which QPU's vary
// 
// 1,7,2     3
// 0,7,2     3
//  - 3-5 QPU's return with only reg's 0-5 filled, which QPU's vary
//
///////////////////////////////////////////////////////////////////////////////
#include <algorithm>  // find()
#include "Support/debug.h"
#include "Driver.h"
#include "v3d.h"
#include "LibSettings.h"

using namespace Log;

namespace V3DLib {
namespace v3d {

namespace {

struct WorkGroup {
  uint32_t wg_x = 0;
  uint32_t wg_y = 0;
  uint32_t wg_z = 0;

  WorkGroup(uint32_t x = 16, uint32_t y = 1, uint32_t z = 1) :
    wg_x(x),
    wg_y(y),
    wg_z(z) {}

  uint32_t wg_size() { return wg_x * wg_y * wg_z; }
};


uint32_t roundup(uint32_t n, uint32_t d) {
	return (n + d -1) /d ;
}

}  // anon namespace


/**
 * Execute a kernel on v3d hardware
 *
 * Source: https://github.com/Idein/py-videocore7/blob/dc4f91d34c428f2d64b10de1738b189abfdf98d9/src/_videocore7/driver.py#L134
 *
 * @param thread number of batches to run
 *
 * @return true if execution went well and no timeout, false otherwise
 *
 * ============================================================================
 * NOTES
 * =====
 *
 * 1. It doesn't appear to be necessary to add the code BO to the bo handles list.
 *    All unit tests pass without doing this.
 *    This is something to keep in mind; it might go awkward later on.
 *
 * 2. Totally no clue what the workgroup if for and what it does.
 *    Can't find anything about it online, just what `py-videocore6` gives,
 *    which I plain took over.
 */
bool Driver::execute(Code &code, Data *uniforms, uint32_t thread) {
  uint32_t code_phyaddr = code.getAddress();
	assert((code_phyaddr & 0x111) == 0);  // Check if there is space for the special flags

  // Technically, you are not required to pass in uniforms.
  // If there are none, set the address to zero.
  uint32_t unif_phyaddr = (uniforms == nullptr)?0u:uniforms->getAddress();
  assertq(m_bo_handles.size() >= 1, "v3d execute: Expecting at least one buffer object on execution");  // See Note 1

	// Special flags
  bool propagate_nan = false;
	bool single_seg    = false;
	bool threading     = false;

	uint32_t special_flags = 0;
	if (propagate_nan) special_flags += (1 << 2);
	if (single_seg)    special_flags += (1 << 1);
	if (threading)     special_flags += 1;

  WorkGroup workgroup;
  uint32_t wgs_per_sg = 16;

	if (!Platform::compiling_for_vc7()) {
		thread--;   // This is what vc6 expects
	}

  st_v3d_submit_csd st = {
    {
      workgroup.wg_x << 16,
      workgroup.wg_y << 16,
      workgroup.wg_z << 16,
      (
			 ((roundup(wgs_per_sg * workgroup.wg_size(), 16u) - 1) << 12) | (wgs_per_sg << 8) | (workgroup.wg_size() & 0xff)
      ),
      thread,

			// Shader address, pnan, singleseg, threading
      code_phyaddr | special_flags,
      unif_phyaddr
    },

		// Not used in the driver
    {0,0,0,0},
    (uint64_t) m_bo_handles.data(),
    (uint32_t) m_bo_handles.size(),
    .in_sync  = 0,
    .out_sync = 0
  };

  //warn << "Timeout: " << LibSettings::qpu_timeout();
  uint64_t timeout_ns = 1000000000llu * LibSettings::qpu_timeout();

  bool ret = (0 == v3d_submit_csd(st));
  assert(ret);
  if (ret) {
    ret = v3d_wait_bo(m_bo_handles, timeout_ns);
  }
  return ret;
}

}  // v3d
}  // V3DLib

#endif  // QPU_MODE
