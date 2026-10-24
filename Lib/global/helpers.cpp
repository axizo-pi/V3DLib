#include "helpers.h"
#include "Log.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem; // Alias for brevity
using namespace Log;
using namespace std;

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

  //Log::warn << "filename: "     << filename;
  //Log::warn << "content size: " << content.length();

  FILE *f = fopen(filename.c_str(), "w");
  Log::assertq(f != nullptr, "to_file() could not open file " + filename);
  fprintf(f, content.c_str());
  fclose(f);

}


/**
 * Source: https://stackoverflow.com/a/525103/1223531
 *
 * Functionally the same as `load_file()`.
 * Consider consolidating (TODO).
 */
std::string from_file(std::string const &filename) {
	ifstream ifs(filename.c_str(), ios::in | ios::binary | ios::ate);

  ifs.seekg(0, ios::end);
  ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, ios::beg);

  vector<char> bytes(fileSize);
  ifs.read(bytes.data(), fileSize);

  return string(bytes.data(), fileSize);
}


std::string load_file(std::string const &filename) {
  std::string ret;

  for (auto const &line : load_file_vec(filename)) {
    ret << line << "\n";
  }

  return ret;
}


std::vector<std::string> load_file_vec(std::string const &filename) {
  std::vector<std::string> ret;

  std::ifstream file(filename);
  assert(file.is_open());

  // Read the file line by line into a string
  std::string line;
  while (getline(file, line)) {
    ret << line;
  }

  file.close();
  return ret;
}


/**
 * @return index of substring in string, -1 if not found
 */
int find_substring(std::string const &str, std::string const &sub, bool find_last) {
  int pos = str.find(sub);

  if (!find_last) {
    if (pos == std::string::npos) return -1;
    return pos;
  }

  int pos2 = pos;
  int tmp;
  while ((tmp = str.find(sub, pos2 + 1)) != std::string::npos) {
    pos2 = tmp;
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
