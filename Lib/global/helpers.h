#ifndef _GLOBAL_HELPERS_H_
#define _GLOBAL_HELPERS_H_
#include <string>
#include <vector>

//
// Basic functions
//
bool ensure_path_exists(std::string const &path);

//
// String functions
//
bool contains(std::string const &s1, std::string const &s2);
std::vector<std::string> split(std::string s, std::string const &delimiter);
std::string indentBy(int indent);

#endif // _GLOBAL_HELPERS_H_
