#ifndef _V3DLIB_VC4_DMA_VPMARRAY_H_
#define _V3DLIB_VC4_DMA_VPMARRAY_H_
#include "Source/Expr.h"

namespace V3DLib {

/**
 * @brief Interface to use VPM as shared memory
 *
 * VPM read/write is slower than you would expect, because it is not a direct memory access.  
 * VPM reads/writes are both put in a request queue and are handled sequentially.  
 * The perfomance is honestly disappointing, but is offset by the utility
 * of having shared memory over the QPU's.
 */
class VPMArray {
public:
  void set(IntExpr const &addr, IntExpr const &val);
  IntExpr get(IntExpr const &addr);

  static void add_nop(); // allow access for the VPM unit test (for now)

private:
  bool m_wait = false;
};


extern VPMArray vpm;

}  // namespace V3DLib

#endif //  _V3DLIB_VC4_DMA_VPMARRAY_H_
