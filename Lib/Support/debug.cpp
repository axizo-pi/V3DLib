#include "debug.h"
#include <iostream>
#include "Exception.h"
#include "global/log.h"

#ifdef DEBUG

void debug_break(const char *str) {
  printf("DEBUG BREAK: %s\n", str);
  breakpoint;
}

#endif  // DEBUG

/**
 * Alternative for `assert` that throws passed string.
 *
 * This is always enabled, also when not building for DEBUG.
 * See header comment of `fatal()` in `basics.h`
 */
void assertq(bool cond, const char *msg, bool do_break) {
  if (cond) {
    return;
  }

  std::string str = "Assertion failed: ";
  str += msg;

#ifdef DEBUG
  if (do_break) {
    std::cout << "assertq(): breakpoint with message: '" << str << "'" << std::endl;
    breakpoint
  }
#endif

  throw V3DLib::Exception(str);
}
