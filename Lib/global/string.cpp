#include "string.h"
#include "global.h"
#include <iostream>

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
