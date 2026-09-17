#include "RegIdSet.h"
#include "basics.h"

namespace V3DLib {

void RegIdSet::add(RegIdSet const &rhs)    { insert(rhs.begin(), rhs.end());     }
void RegIdSet::remove(RegIdSet const &rhs) { erase(rhs.begin(), rhs.end());      }
int RegIdSet::first() const                { assert(!empty()); return *cbegin(); }


std::string RegIdSet::dump() const {
  std::string ret;

  ret << "(";

	bool did_first = false;
  for (auto reg : *this) {
    ret << reg;

		if (did_first) {
    	ret << ", ";
		}
		did_first = true;
  }

  ret << ")";

  return ret;
}

}  // namespace V3DLib
