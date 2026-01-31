#ifndef _V3DLIB_SUPPORT_HELPERS_H_
#define _V3DLIB_SUPPORT_HELPERS_H_
#include <string>

namespace V3DLib {

float random_float();
std::string indentBy(int indent);
void to_file(std::string const &filename, std::string const &content);
void sleep(int sec);
bool contains(std::string const &s1, std::string const &s2);	

}  // namespace V3DLib

#endif  // _V3DLIB_SUPPORT_HELPERS_H_
