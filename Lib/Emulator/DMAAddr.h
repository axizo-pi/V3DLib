#ifndef _V3DLIB_EMULATOR_DMAADDR_H_
#define _V3DLIB_EMULATOR_DMAADDR_H_
#include "Target/SmallLiteral.h"  // Word

namespace V3DLib {

/**
 * @brief In-flight DMA request
 */
struct DMAAddr {
  Word addr;

  bool active() const        { return m_active; }
  void active(bool val)      { m_active = val; }
  void start();
  bool waiting() const       { return m_wait_count > 0; }
  bool done_waiting() const  { return active() && m_wait_count == 0; }

  void upkeep();
  std::string dump() const;

private:
  const int WaitCount = 4;  // Educated quess

  bool m_active     = false;
  int  m_wait_count = 0;
};

} // namespace V3DLib

#endif  //  _V3DLIB_EMULATOR_DMAADDR_H_
