#ifndef _GLOBAL_STRING_H
#define _GLOBAL_STRING_H
#include <string>
#include <vector>

//
// String functions
//
namespace string {

bool contains(std::string const &s1, std::string const &s2);
std::vector<std::string> split(std::string s, const std::string& delimiter, bool do_all = true);
std::string indentBy(int indent);
bool begins_with(std::string const &rhs, std::string const &prefix);
bool hasEnding (std::string const &fullString, std::string const &ending);
std::string trim(std::string const &s);
std::string pad(std::string const &s, int width, bool do_left = true);
std::string pad(int val, int width = 10);
int find_substring(std::string const &str, std::string const &sub, bool find_last = false);
bool has_whitespace(std::string const &str);
std::string rep(std::string const &rep, int repeats);

std::string implode( const std::vector<std::string>& elements, const char* const separator = ", ");

} // namespace string

#endif //  _GLOBAL_STRING_H
