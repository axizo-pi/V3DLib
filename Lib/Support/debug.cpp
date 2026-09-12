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
