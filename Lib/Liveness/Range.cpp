#include "Range.h"
#include "Support/basics.h"

namespace V3DLib {

Range::Range(int first, int last) :
  m_first(first),
  m_last(last)
{
  assert(first >= 0);
  assert(last >= 0);
  assert(first <= last);
}


void Range::add(int val) {
  assert(val >= 0);  // Not expecting negative values for now

  if (m_first == -1 || m_first > val) {
    m_first = val;
  }

  if (m_last == -1 || m_last < val) {
    m_last = val;
  }
}


int Range::first() const {
  return m_first;
}


int Range::last() const {
  assert(m_last == -1 || m_last >= m_first);
  return m_last;
}


int Range::range() const {
  if (empty()) return 0;
  return (m_last - m_first + 1);
}


void Range::reset() {
  m_first = -1;
  m_last  = -1;
}


bool Range::empty() const {
  return (m_first == -1 && m_last == -1);
}


bool Range::in(int rhs) const {
  if (empty()) return false;
  return m_first <= rhs && rhs <= m_last;
}


bool Range::overlaps(Range const &rhs) const {
  assert(!empty());
  assert(!rhs.empty());

  return !(rhs.last() < first() || rhs.first() > last());
}


bool Range::is_embedded(Range const &rhs) const {
  assert(!empty());
  assert(!rhs.empty());

  if (first() < rhs.first() && last() >= rhs.last()) {
    return true;
  }

/*
  // Check fails when this block is embedded in rhs

  if (overlaps(rhs)) {
    std::string msg;

    msg << "rhs block is not embedded in this block but DOES overlap.\n"
        << "this block: " << dump() << ", rhs block: " << rhs.dump();
    assertq(false, msg);
}
*/

  return false;
}


std::string Range::dump() const {
  std::string ret;

  if (empty()) {
    ret << "none";
  } else {
    ret << m_first << ", " << m_last;
  }

  return ret;
}


/**
 * @brief Assign a range without doing bounds checking.
 */
void Range::unsafe_assign(int first, int last) {
  m_first = first;
  m_last  = last;
}

}  // namespace V3DLib
