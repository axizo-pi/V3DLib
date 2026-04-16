#ifndef _V3DLIB_SUPPORT_DEBUG_H
#define _V3DLIB_SUPPORT_DEBUG_H
#include <signal.h>  // raise(SIGTRAP)
#include <string>

#if defined __cplusplus
#include <cassert>
#else
#include <assert.h>
#endif

#ifdef DEBUG

#define breakpoint raise(SIGTRAP);

void debug_break(const char *str);

#else

#define breakpoint

inline void debug_break(const char *str) {}

#endif  // DEBUG

#define MAYBE_UNUSED __attribute__((unused))

#endif  // _V3DLIB_SUPPORT_DEBUG_H
