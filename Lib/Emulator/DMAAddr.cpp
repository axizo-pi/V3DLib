#include "DMAAddr.h"
#include "Support/basics.h"

namespace V3DLib {

void DMAAddr::start() {
  assert(!m_active);
  m_active = true;
  m_wait_count = WaitCount;
}

void DMAAddr::upkeep() {
  if (m_active) {
    if (m_wait_count > 0) m_wait_count--;
    //warn << "DMAAddr wait " << m_wait_count;
  }
}


std::string DMAAddr::dump() const {
  std::string ret;

  if (m_active) ret << "active, ";
  ret << "count : " << m_wait_count;

  return ret;
}



} // namespace V3DLib


