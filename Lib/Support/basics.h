#ifndef _V3DLIB_SUPPORT_BASICS_H
#define _V3DLIB_SUPPORT_BASICS_H
#include "global/global.h"
#include <stdint.h>

using namespace Log;

namespace V3DLib {

/**
 * @brief Terminate the application
 *
 * This happens by throwing an exception, rather than abruptly closing with `exit()` or `abort()`.
 * This allows the stack to unwind and all objects clean up behind them, also the global objects.
 *
 * This is significant for Buffer Objects, which are system-global resource. If these are not properly
 * deallocated, eventually the memory will clog up and the GPU can't be used any more.
 * This is an issue notably with vc4. The v3d has a higher tolerance level, but the issue is real there
 * as well.
 */
inline void fatal(const char *msg) {
  std::string str = "FATAL: ";
  str += msg;
  throw Exception(str);
}


inline void fatal(std::string const &msg) {
  fatal(msg.c_str());
}

}  // V3DLib


void findAndReplaceAll(std::string &data, std::string toSearch, std::string replaceStr);

std::string tabbed(int tab_size, std::string const &val);
std::string tabbed(int tab_size, int val);
inline std::string tabs(int tab_size) { return tabbed(tab_size, ""); }

std::string title(std::string const &str);

#endif  // _V3DLIB_SUPPORT_BASICS_H
