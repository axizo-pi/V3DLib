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

std::string sudo();
bool ensure_path_exists(std::string const &path);
bool ensure_file_exists(std::string const &path);
float random_float();
void sleep(int sec);

//
// String functions
//
std::string indentBy(int indent);
bool contains(std::string const &s1, std::string const &s2);
bool hasEnding (std::string const &fullString, std::string const &ending);
std::vector<std::string> split(std::string s, std::string const &delimiter);
int num_newlines(std::string const &s);

//
// Other functions
//
bool uniforms_reversed();

}  // namespace V3DLib

#endif  // _V3DLIB_SUPPORT_HELPERS_H_
