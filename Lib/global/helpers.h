#ifndef _GLOBAL_HELPERS_H_
#define _GLOBAL_HELPERS_H_
#include <string>
#include <vector>

//
// Basic functions
//
bool ensure_path_exists(std::string const &path);
void to_file(std::string const &filename, std::string const &content);
std::string from_file(std::string const &filename);
std::string load_file(std::string const &filename);
std::vector<std::string> load_file_vec(std::string const &filename);

#endif // _GLOBAL_HELPERS_H_
