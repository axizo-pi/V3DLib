#include "VPMArray.h"
#include "Operations.h"
#include "Source/Functions.h"  // nop(int)

namespace V3DLib {

/**
 * ==================================================
 * Examination
 * -----------
 *
 * vc4 only.
 *
 * * pi3
 *   + 2 QPU's, count:
 *     - 10: #1 filled, #0 sometimes
 *     - 30: all filled
 *   + 4 QPU's, count:
 *     - 70: all filled
 *     - 60: #0, #1, #3 filled
 *     - 50: #0, #1, #3 filled
 *     - 40: #0, #1, #3 filled
 *     - 30: #0, #3 filled (once all filled)
 *     - 20: #0, #3 filled
 *     - 10: only #3 filled
 *     -  0: only #3 filled
 *   + 8 QPU's, count:
 *     - 80: all filled except #2
 *     - 70: all filled except #2
 * * Pi2
 *   + 4 QPU's, count:
 *     - 100: all filled
 *     -  90: all filled, #2 fails sometimes
 * * Pi1
 *   + 4 QPU's, count:
 *     - 100: all filled
 *     -  90: all filled, #2 fails sporadically
 *     -  80: #0, #1, #3 filled, #2 fails
 * * zero
 *   + 4 QPU's, count:
 *     - 90: all filled
 *     - 80 #2 not filled, rest filled
 *     - 70 #2 not filled, rest filled
 */
void VPMArray::add_nop() {
  if (Platform::use_main_memory()) return; // Compiling for emulator, don't bother

  int count = 80;

  // These are best value for #QPU=4. Other #QPU may need different values
  switch (Platform::tag()) {
    case Platform::pi1:     count = 100; break;
    case Platform::pi2:     count = 100; break;
    case Platform::pi_zero: count =  90; break;
    default: /* Use default */           break;
  }

  nop(count);
}

/**
 * @param addr  index in VPM memory to write to.
 *              The index is over vector values.
 */
void VPMArray::set(IntExpr const &addr, IntExpr const &val) {
  vpmSetupWrite(HORIZ, addr);
  vpmPut(val);

  m_wait = true;
}


/**
 * @param addr  index in VPM memory to read from.
 *              The index is over vector values.
 */
IntExpr VPMArray::get(IntExpr const &addr) {
  if (m_wait) {
    //
    // The implementation implies that the current QPU is waiting for its own
    // write to complete.
    //
    // This is NOT the case; in actual case the QPU is waiting for any OTHER
    // QPU to complete their write.
    //
    // The assumption here is that all QPU's do the same write, so that the execution
    // is comparable.
    //
    add_nop();
    m_wait = false;
  }

  vpmSetupRead(HORIZ, 1, addr);
  return vpmGetInt();
}


VPMArray vpm;

}  // namespace V3DLib

