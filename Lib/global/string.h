#ifndef _GLOBAL_STRING_H
#define _GLOBAL_STRING_H
#include <string>
#include <vector>

bool contains(std::string const &s1, std::string const &s2);
std::vector<std::string> split(std::string s, const std::string& delimiter, bool do_all);
std::string indentBy(int indent);

#endif //  _GLOBAL_STRING_H
