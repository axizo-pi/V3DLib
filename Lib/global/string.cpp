#include "string.h"
#include "global.h"
#include <iostream>
#include <algorithm>  // find_if()

namespace {

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
 *     
 *     split_a(s, delimiter, true); // ["scott", "tiger", "mushroom"]
 *
 * @param do_all If false, return only first split. Otherwise, return all splits.
 */
std::vector<std::string> split_a(std::string s, const std::string& delimiter, bool do_all) {
  std::vector<std::string> tokens;
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


/**
 * @brief Split a string into substrings at a given delimiter.
 *
 * `s` is destroyed internally
 *
 * Source: https://stackoverflow.com/a/14266139/1223531
 *
 * @param s         String to split.
 * @param delimiter Substring to split on.
 * @return          Array of strings split on delimiter. The delimiter is left out.
 */
std::vector<std::string> split_b(std::string s, std::string const &delimiter) {
  std::vector<std::string> tokens;
  size_t pos = 0;
  std::string token;
  //int count = 0;

  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
    //count++;
  }

  // Following adds any part at the end, which does not end with delimiter
  if (!s.empty()) {
    tokens.push_back(s);
  }

  //std::cout << "count: " << count << "\n";
  //std::cout << "size : " << tokens.size() << "\n";
  return tokens;
}


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
  if (s1.find(s2) != std::string::npos) {
    return true;
  }

  return false;
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
 * @brief Adds timing and logging to split function.
 *
 * See `split_a()`
 */
std::vector<std::string> split(std::string s, const std::string& delimiter, bool do_all) {
  std::vector<std::string> ret;

  if (s.empty()) { // Nothing to split
    ret.push_back(s);
    return ret;
  }

  timers.start("split_a");
  ret = split_a(s, delimiter, do_all);
  timers.stop("split_a");

  timers.start("split_b");
  auto v1 = split_b(s, delimiter);
  timers.stop("split_b");

  // Can NOT write to log here, because split() is used within Log

  if ((do_all && ret != v1) || ret[0] != v1[0]) {
    std::cout << "split_c difference:\n"
              << "  input: '" << s << "', '" << delimiter << "', doall: " << do_all << "\n"
              << "  ret  : " << dump(ret) << "\n"
              << "  v1   : " << dump(v1)  << "\n";
  }

  return ret;
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
 * @brief Left-pad string to specified width
 */
std::string pad(std::string const &s, int width) {
  assert(width >= 0);
  int w = width - (int) s.length();

  std::string ret;
  ret << indentBy(w) << s;
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
