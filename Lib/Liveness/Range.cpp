#include "Range.h"
#include "Support/basics.h"

namespace V3DLib {

Range::Range(int first, int last) :
  m_first(first),
  m_last(last),
  m_count(first - last + 1)  // This is dubious; count appears to be number of items in range
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

  m_count++;
}


int Range::first() const {
  return m_first;
}


int Range::last() const {
  assert(m_last == -1 || m_last >= m_first);
  return m_last;
}


int Range::count() const {
  assert(m_count == 0 || !empty());
  return m_count;
}


int Range::range() const {
  if (empty()) return 0;
  return (m_last - m_first + 1);
}


void Range::reset() {
  m_first = -1;
  m_last  = -1;
  m_count =  0;
}


bool Range::empty() const {
  if (m_first == -1 && m_last == -1) {
    assert(m_count == 0);
    return true;
  }

/*
  // Count is # items in range, not size of range.
  // It is perfectly possible to have a range without items in it
  bool ok = (m_first != -1 && m_last != -1 && m_count != 0);
  if (!ok) {
    warn << "Range::empty: (" << m_first << ", " << m_last << ", " << m_count << ")"; 
    assert(false);
  }
*/
  return false;
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
    ret << m_first << ", " << m_last << ", " << m_count;
  }

  return ret;
}


/**
 * **Note:** Count is number of items in range, not size of range
 */
void Range::unsafe_assign(int first, int last) {
  m_first = first;
  m_last  = last;
}

}  // namespace V3DLib
