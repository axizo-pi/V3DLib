#ifndef _V3DLIB_LIVENESS_RANGE_H_
#define _V3DLIB_LIVENESS_RANGE_H_
#include <string>

namespace V3DLib {

/**
 * Local helper class for keeping track of integer ranges
 */
class Range {
public:
  Range() = default;
  Range(int first, int last);

  void add(int val);
  int first() const;
  int last() const;
  int count() const;
  int range() const;
  bool empty() const;
  bool in(int rhs) const;
  bool overlaps(Range const &rhs) const;
  bool is_embedded(Range const &rhs) const;
  std::string dump() const;
  void reset();

  void unsafe_assign(int first, int last);

private:
  int m_first = -1;
  int m_last  = -1;
  int m_count =  0;  // Number of items within this range
};

}  // namespace V3DLib

#endif  // _V3DLIB_LIVENESS_RANGE_H_
