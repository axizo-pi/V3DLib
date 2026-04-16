#include "Helpers.h"
#include "Support/basics.h"
#include "Support/Helpers.h"
#include "Support/Platform.h"
#include <filesystem>
#include <thread>
#include <fstream>

namespace fs = std::filesystem; // Alias for brevity

/** \file 
 * Helper Functions
 * ================
 *
 * Collection of useful functions which are unrelated to the object of the library.
 */

using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono;      // nanoseconds, system_clock, seconds

namespace V3DLib {
namespace {

/**
 * Adjusted from opcodes() in vc4/Instr.cpp
 */
std::vector<std::string> exec(std::string const &command) {
  std::vector<std::string> ret;

  std::string tmp_file;
  tmp_file << fs::temp_directory_path() << "/system_tmp.txt";

  std::string cmd = command + " > " + tmp_file;
  int result = std::system(cmd.c_str());
  assert(result == 0);

  ret = load_file_vec(tmp_file);

/*
  std::string buf;
  for (auto const &s: ret) {
    buf << s << "\n";
  }
  warn << "system() returns:\n" << buf;
*/  

  std::remove(tmp_file.c_str());
  return ret;
}


struct platform_info {
  bool is_debian = false;
  int  version   = -1;
  bool is_arm    = false;

  std::string dump() const {
    std::string ret = "\n";

    ret << "  is_debian: " << is_debian << "\n"
        << "  version  : " << version << "\n"
        << "  is_arm   : " << is_arm;

    return ret;
  }
};


/**
 * @brief retrieve essential platform info
 */
platform_info  get_platform_info() {
  platform_info p_i;

  auto ret = exec("hostnamectl");

  for (auto const &s: ret) {
    if (contains(s, "Operating System:")) {
      // e.g. 'Operating System: Debian GNU/Linux 13 (trixie)'

      p_i.is_debian = contains(s, "Debian");
      auto tmp = split(s, " ");

      // Get next-to last item, which is the version number
      int index = (int) tmp.size() - 2;
      p_i.version = atoi(tmp[index].c_str());
    }

    if (contains(s, "Architecture:")) {
      p_i.is_arm = contains(s, "arm");
    }
  }

  //warn << "platform_info:" << p_i.dump();
  return p_i;
}


/**
 * @brief display gnu c++ version.
 *
 * @return main version number
 */
MAYBE_UNUSED int gnu_version(bool show = true) {
  if (show) {
    warn << "Gnu C++ version: " 
         << __GNUC__       << "."
         << __GNUC_MINOR__ << "."
         << __GNUC_PATCHLEVEL__;
  }

  return __GNUC__;
}

} // anon namespace


/**
 * @brief Determine if sudo is required
 *
 * @return `sudo` if required, empty string otherwise
 *
 */
std::string sudo_prefix() {
  std::string ret;

#ifdef QPU_MODE
const char *SUDO = (V3DLib::Platform::run_vc4())? "sudo " : "";  // sudo needed for vc4
#else
const char *SUDO = "";
#endif

  ret << SUDO;

  //warn << "sudo_prefix() ret:\n" << ret;
  return ret;
}


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

  //Log::warn << "filename: "     << filename;
  //Log::warn << "content size: " << content.length();

  FILE *f = fopen(filename.c_str(), "w");
  Log::assertq(f != nullptr, "to_file() could not open file " + filename);
  fprintf(f, content.c_str());
  fclose(f);

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


std::string load_file(std::string const &filename) {
  std::string ret;

  for (auto const &line : load_file_vec(filename)) {
    ret << line << "\n";
  }

  return ret;
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
    return true;
  }

  return false;
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
  return (int) (ret.size() - 1);
}


/**
 * @brief Detect if the parameters (i.e. uniforms) are reversed on current platform.
 *
 * On `x86`, the kernel parameters are reversed.
 * Thus, last parameter in the kernel function call is initialized first.
 * This screws up initialization of the uniforms in the kernel.
 *
 * @return true if uniforms reversed, false otherwise
 *
 * =========================================
 *
 * Notes
 * -----
 *
 * The original hypothesis was  that the parameter reversal occured with gnu c++ v14.2.0.
 * This is not true; given compiler running on `Pi3B+` with `Debian 13 (Trixie) arm64` does not 
 * reverse.
 *
 * So, it must be the hardware platform, i.e. `x86`. This is the current hypothesis.
 */
bool uniforms_reversed() {
  static bool showed_msg = false;
  auto ret = get_platform_info();

  if (!showed_msg) {
    warn << "platform_info: " << ret.dump();

    if (ret.is_arm) {  // Actually, reversal appears to happen explicitly on x64
      warn << "No need to reverse the parameter indexes";
    }

    showed_msg = true;
  }

  return !ret.is_arm;
}

}  // namespace V3DLib
