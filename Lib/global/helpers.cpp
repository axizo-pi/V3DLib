#include "helpers.h"
#include "log.h"
#include <filesystem>

namespace fs = std::filesystem; // Alias for brevity

using namespace Log;


/**
 * @brief Create a path with read/write permissions, if not already present
 *
 * @param path  path to create, if not present
 * @return      true if call succeeded, false otherwise
 */
bool ensure_path_exists(std::string const &path) {
  assert(!path.empty());

  std::string cmd;

  if (!fs::exists(path)) {
    if (!fs::create_directories(path)) {
      cerr << "ensure_path_exists() creation of path '" << path << "' failed.";
      return false;
    }

    // Change the file permissions to read/write
    fs::permissions(
       path,
       fs::perms::owner_all | fs::perms::group_all | fs::perms::others_all,
       fs::perm_options::replace
     );
  }

  return true;
}


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
std::vector<std::string> split(std::string s, std::string const &delimiter) {
  //std::cout << "split: \"" << s << "\"" << "\n";

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
