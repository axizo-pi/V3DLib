#ifndef _GLOBAL_STRING_H
#define _GLOBAL_STRING_H
#include <string>
#include <vector>

//
// String functions
//
bool contains(std::string const &s1, std::string const &s2);
std::vector<std::string> split(std::string s, const std::string& delimiter, bool do_all);
std::string indentBy(int indent);
bool begins_with(std::string const &rhs, std::string const &prefix);
bool hasEnding (std::string const &fullString, std::string const &ending);
std::string trim(std::string const &s);
std::string pad(std::string const &s, int width);
std::string pad(int val, int width = 10);

#endif //  _GLOBAL_STRING_H
