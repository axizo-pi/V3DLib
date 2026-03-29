#ifndef _LIB_SUPPORT_REGIDSET_H
#define _LIB_SUPPORT_REGIDSET_H
#include <set>
#include <string>

namespace V3DLib {

/**
 * This is actually just a set of integers, so the name is strictly speaking wrong.
 * But I will leave the name because it shows the intent.
 */
class RegIdSet : public std::set<int> {
public:
  void add(RegIdSet const &rhs);
  void remove(RegIdSet const &rhs);
  void remove(int rhs) { erase(rhs); }
  bool member(int rhs) const { return (find(rhs) != cend()); }
  int first() const;
  std::string dump() const;
};

}  // namespace V3DLib

#endif  // _LIB_SUPPORT_REGIDSET_H
