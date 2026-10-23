#include "debug.h"
#include "Log.h"
#include <iostream>

#ifdef DEBUG

void debug_break(const char *str) {
  printf("DEBUG BREAK: %s\n", str);
  breakpoint;
}

#endif  // DEBUG
