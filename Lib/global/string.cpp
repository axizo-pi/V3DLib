#include "string.h"
#include "global.h"
#include <iostream>
#include <iterator>
#include <algorithm>  // find_if()

namespace string {
namespace {

/**
 * @brief Trim from the start (in place)
 *
 * Source: https://stackoverflow.com/questions/216823/how-can-i-trim-a-stdstring
 */
void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
}


/**
 * @brief Trim from the end (in place)
 *
 * Source: https://stackoverflow.com/questions/216823/how-can-i-trim-a-stdstring
 */
void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}


/**
 * @brief Trim input string
 *
 * This changes the input string.
 */
void trim_d(std::string &s) {
  rtrim(s);
  ltrim(s);
}

} // anon namespace


/**
 * @brief Check if a given string contains a substring.
 *
 * This is actually a one-liner in modern C++,
 * But defining it like this indicates the intent.
 *
 * @param s1 String to scan for substring
 * @param s2 Substring to scan for.
 */
bool contains(std::string const &s1, std::string const &s2) {  
  return (s1.find(s2) != std::string::npos);
}


/**
 * Return a string containing the passed number of spaces.
 *
 * @param indent Number of spaces to put in string.
 */
std::string indentBy(int indent) {
  std::string ret;

  if (indent < 0) return ret;

  for (int i = 0; i < indent; i++) ret += " ";
  return ret;
}


/**
 * @brief Split string into substring at the delimiters in the string (if any).
 *
 * If no delimiter present, two substrings are returned with the second empty
 *
 * Source: https://stackoverflow.com/a/14266139/1223531
 *
 * Example:
 *
 *     std::string s = "scott>=tiger>=mushroom";
 *     std::string delimiter = ">=";
 *     split_a(s, delimiter, true); // ["scott", "tiger", "mushroom"]
 *
 * @param do_all If false, return only first split. Otherwise, return all splits.
 *
 * ===============================================================
 * Notes
 * -----
 *
 * 1. Can NOT write to `Log` in this function, because `split()` is used within `Log`.
 */
std::vector<std::string> split(std::string s, const std::string& delimiter, bool do_all) {
  std::vector<std::string> tokens;

  if (s.empty()) { // Nothing to split
    tokens.push_back(s);
    return tokens;
  }

  size_t pos = 0;
  std::string token;

  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());

    if (!do_all) break; // only do a single split
  }

  // Following adds any part at the end, which does not end with delimiter
  if (!s.empty()) {
    tokens.push_back(s);
  }

  return tokens;
}


bool begins_with(std::string const &rhs, std::string const &prefix) {
  return (rhs.rfind(prefix, 0) == 0);
}


/**
 * Source: https://stackoverflow.com/a/874160/1223531
 */
bool hasEnding (std::string const &fullString, std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
  } else {
    return false;
  }
}


/**
 * @brief Version of trim which does not change the input string
 */
std::string trim(std::string const &s) {
  std::string tmp = s;
  trim_d(tmp);
  return tmp;
}


/**
 * @brief pad string to specified width
 *
 * @param do_left If true (default), left-pad the input string.
 *                Otherwise, right-pad.
 */
std::string pad(std::string const &s, int width, bool do_left) {
  assert(width >= 0);
  int w = width - (int) s.length();

  std::string ret;

  if (do_left) {
    ret << indentBy(w) << s;
  } else {
    ret << s << indentBy(w);
  }
  return ret;
}


/**
 * @brief Left-pad integer string output to specified width.
 *
 * Default value `width = 10` is decent for integer values.
 */
std::string pad(int val, int width) {
  std::string buf;
  buf << val;
  return pad(buf, width);
}


/**
 * @brief repeat the input string the given number of times.
 */
std::string rep(std::string const &rep, int repeats) {
  std::string ret;

  for (int i = 0; i < repeats; ++i) {
    ret << rep;
  }

  return ret;
}


/**
 * Source: http://stackoverflow.com/questions/5288396/c-ostream-out-manipulation/5289170#5289170
 */
std::string implode(const std::vector<std::string>& elements, const char* const separator) {
  switch (elements.size()) {
  case 0:
    return "";
  case 1:
    return elements[0];
  default:
    std::ostringstream os;
    copy(elements.begin(), elements.end() - 1, std::ostream_iterator<std::string>(os, separator));
    os << *elements.rbegin();
    return os.str();
  }
}


/**
 * @return index of substring in string, -1 if not found
 */
int find_substring(std::string const &str, std::string const &sub, bool find_last) {
  int pos = (int) str.find(sub);

  if (!find_last) {
    if (pos == (int) std::string::npos) return -1;
    return pos;
  }

  int pos2 = pos;
  long unsigned tmp;
  while ((tmp = str.find(sub, pos2 + 1)) != std::string::npos) {
    pos2 = (int) tmp;
  }

  return pos2;
}


bool has_whitespace(const std::string &str) {
  if (str.find(" ")  != std::string::npos) return true;
  if (str.find("\t") != std::string::npos) return true;
  if (str.find("\r") != std::string::npos) return true;
  if (str.find("\n") != std::string::npos) return true;

  return false;
}

} // namespace string
