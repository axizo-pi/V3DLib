#ifndef _V3DLIB_SUPPORT_HELPERS_H_
#define _V3DLIB_SUPPORT_HELPERS_H_
#include "global/global.h"
#include <string>
#include <vector>

namespace V3DLib {

//
// Basic functions
//
std::string sudo_prefix();
float random_float();
void sleep(int sec);
int resize_16(int in_val, bool do_dump = false);

//
// String functions
//
int num_newlines(std::string const &s);
int num_empty(std::string const &s, std::string const prefix = "");

//
// Debug Functions
//
int bit_diff(float in_val1, float in_val2, int ignore_bit = 0);

}  // namespace V3DLib

#endif  // _V3DLIB_SUPPORT_HELPERS_H_
