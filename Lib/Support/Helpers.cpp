#include "Helpers.h"
#include "Support/basics.h"
#include <chrono>
#include <thread>

/** \file 
 * Helper Functions
 * ================
 *
 * Collection of useful functions which are unrelated to the object of the library.
 */

using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono;      // nanoseconds, system_clock, seconds

namespace V3DLib {

/**
 * Return a random float value between -1.0f and 1.0f.
 *
 * @return Value between -1.0f and 1.0f
 */ 
float random_float() {
  return  (1.0f*(((float) (rand() % 200)) - 100.0f))/100.0f;
}


/**
 * Return a string containing the passed number of spaces.
 *
 * @param indent Number of spaces to put in string.
 */
std::string indentBy(int indent) {
  std::string ret;
  for (int i = 0; i < indent; i++) ret += " ";
  return ret;
}


/**
 * Write the passed string to a file with the passed filename.
 *
 * The goal of this function is to simplify writing to file.  
 * This implementation avoids having to pass a filename to any code
 * that wants to write to file. In addition, string output can be used
 * for other purposes, eg. logging.
 *
 * @param filename Name of file to write to.
 * @param content  String buffer to write to file.
 */
void to_file(std::string const &filename, std::string const &content) {
  assert(!filename.empty());
  assert(!content.empty());

  FILE *f = fopen(filename.c_str(), "w");
   assert (f != nullptr);
  fprintf(f, content.c_str());
  fclose(f);
}


/**
 * Suspend execution for the given number of seconds.
 *
 * @param sec Number of seconds to wait.
 */
void sleep(int sec) {
  sleep_for(seconds(sec));
}


/**
 * Check if a given string contains a substring.
 *
 * This is actually a one-liner in modern C++,
 * But defining it like this indicates the intent.
 *
 * @param s1 String to scan for substring
 * @param s2 Substring to scan for.
 */
bool contains(std::string const &s1, std::string const &s2) {  
  if (s1.find(s2) != std::string::npos) {
    //std::cout << "found!" << '\n';
    return true;
  }

  return false;
}


/**
 * Split a string into substrings at a given delimiter.
 *
 * Source: https://stackoverflow.com/a/14266139/1223531
 *
 * @param s         String to split.
 * @param delimiter Substring to split on.
 * @return          Array of strings split on delimiter. The delimiter is left out.
 */
std::vector<std::string> split(std::string s, std::string const &delimiter) {
  // s is destroyed internally

  std::vector<std::string> tokens;
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
  }
  tokens.push_back(s);

  return tokens;
}


int num_newlines(std::string const &s) {
  auto ret = split(s, "\n");
  return (ret.size() - 1);
}


 

}  // namespace V3DLib
