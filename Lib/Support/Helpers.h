#ifndef _V3DLIB_SUPPORT_HELPERS_H_
#define _V3DLIB_SUPPORT_HELPERS_H_
#include <string>
#include <vector>

namespace V3DLib {

//
// Basic functions
//
void to_file(std::string const &filename, std::string const &content);
std::vector<std::string> load_file_vec(std::string const &filename);
std::string load_file(std::string const &filename);

std::string sudo_prefix();
bool ensure_path_exists(std::string const &path);
float random_float();
void sleep(int sec);
int resize_16(int in_val, bool do_dump = false);

//
// String functions
//
std::string indentBy(int indent);
bool contains(std::string const &s1, std::string const &s2);
bool hasEnding (std::string const &fullString, std::string const &ending);
std::vector<std::string> split(std::string s, std::string const &delimiter);
int num_newlines(std::string const &s);
int num_empty(std::string const &s, std::string const prefix = "");
void trim(std::string &s);
std::string trim_s(std::string const &s);

//
// Debug Functions
//
int bit_diff(float in_val1, float in_val2, int ignore_bit = 0);

}  // namespace V3DLib

#endif  // _V3DLIB_SUPPORT_HELPERS_H_
