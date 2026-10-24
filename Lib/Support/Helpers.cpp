#include "Helpers.h"
#include "Support/basics.h"
#include "Support/Helpers.h"
#include "Support/Platform.h"
#include <filesystem>
#include <thread>
//#include <fstream>

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
  std::remove(tmp_file.c_str());

  return ret;
}


struct platform_info {
  bool is_debian = false;
  int  version   = -1;
  std::string architecture;

  bool is_arm() const { return contains(architecture, "arm"); }
  bool is_x86() const { return contains(architecture, "x86"); }

  std::string dump() const {
    std::string ret = "\n";

    ret << "  is_debian   : " << is_debian    << "\n"
        << "  version     : " << version      << "\n"
        << "  architecture: " << architecture << "\n"
        << "  is_arm()    : " << is_arm()   << "\n"
        << "  is_x86()    : " << is_x86();

    return ret;
  }
};


/**
 * @brief retrieve essential platform info
 */
MAYBE_UNUSED platform_info get_platform_info() {
  platform_info p_i;

  auto ret = exec("hostnamectl");

  for (auto const &s: ret) {
    if (contains(s, "Operating System:")) {
      // e.g. 'Operating System: Debian GNU/Linux 13 (trixie)'

      // Raspbian counts as debian
      p_i.is_debian = contains(s, "Debian") || contains(s, "Raspbian");
      auto tmp = split(s, " ", true);

      // Get next-to last item, which is the version number
      int index = (int) tmp.size() - 2;
      p_i.version = atoi(tmp[index].c_str());
    }

    if (contains(s, "Architecture:")) {
      auto tmp = split(s, ": ", true);
      p_i.architecture = tmp[1];
    }
  }

  static bool showed_msg = false;
  if (!showed_msg) {
    info << "platform_info:" << p_i.dump();
    showed_msg = true;
  }

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
  const char *SUDO = (V3DLib::Platform::hw::vc4())? "sudo " : "";  // sudo needed for vc4
  ret << SUDO;

  //warn << "sudo_prefix() ret:\n" << ret;
  return ret;
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
 * Suspend execution for the given number of seconds.
 *
 * @param sec Number of seconds to wait.
 */
void sleep(int sec) {
  sleep_for(seconds(sec));
}


int num_newlines(std::string const &s) {
  auto tmp = split(s, "\n", true);
  return (int) (tmp.size() - 1);
}


/**
 * If `prefix` specified, regard the lines starting with prefix as empty.
 */
int num_empty(std::string const &s, std::string const prefix) {
  int ret = 0;

  if (!prefix.empty()) {
    auto tmp = split(s, ("\n" + prefix), true);
    ret = (int) (tmp.size() - 1);
  }

  auto tmp = split(s, "\n", true);
  for (int i = 0; i < (int) tmp.size(); i++) {
    if (tmp[i].empty()) {
      ret++;
    }
  }

  return ret;
}


/**
 * @brief Determine the first bit that differs in the parameters.
 *
 * If the difference falls outside of the specified bit margin,
 * the top bit index of the offset is determined.
 *
 * The basic test is:
 *
 *     abs((uint) val1 - (uint) val2) <= (1 << ignore_bit)
 *
 * The conversion to `uint` is bit-wise.
 *
 * The result is the regular bit offset, where bit 0 is the least
 * significant bit of the fraction.
 *
 * @param in_val1    First value to check
 * @param in_val2    Second value to check
 * @param ignore_bit Index of tpmost bit to ignore.
 *                   What is tested, is the range which the bit represents.
 *                   (e.g. for `ignore_bit = 1`, a difference of 2 is allowed.
 *                   The default value is 0, meaning that the least significant
 *                   bit does not count. This occurs often.
 *                   To do an exact compare, pass -1 for this parameter.
 * @return           index of first bit that is different, -1 if values deemed the same
 *
 * ===============================
 * Notes
 * -----
 *
 * 1. Rollover issue:
 *
 *   | val1       | val2       | diff | diff bit |
 *   |------------|------------|------|----------|
 *   | 0x397c20d8 | 0x397c20d7 | 1    |  3       |
 *   | 0x397c4950 | 0x397c494f | 1    |  4       |
 *   | 0x39c119ff | 0x39c11a00 | 1    |  9       |
 *   | 0x397c8000 | 0x397c7fff | 1    | 15       |
 *   |------------|------------|------|----------|
 *
 *   Even if the value diff is well within the mask range, the top changed bit
 *   can be much larger. Better to use margins instead of masks.
 */
int bit_diff(float in_val1, float in_val2, int ignore_bit) {
  assert(sizeof(uint32_t) == sizeof(float));
  int size = (int) 8*sizeof(uint32_t);
  assert(-1 <= ignore_bit && ignore_bit < size);
  int offset = -1;

  uint32_t val1 = *((uint32_t *) &in_val1);
  uint32_t val2 = *((uint32_t *) &in_val2);

  // Special handling for `-0`, which occurs on vc4.
  // Convert '-0' to '0'.
  if (val1 == 0x80000000) val1 = 0;  // Doesn't appear to occur, taking into account anyway
  if (val2 == 0x80000000) val2 = 0;

  uint32_t bit_margin = 0;
  if (ignore_bit != -1) {
    bit_margin = 1 << ignore_bit;
  }

  uint32_t abs_diff = (val1 > val2)? (val1 - val2): (val2 - val1);

  bool ok = (abs_diff <= bit_margin);
  if (ok) return offset;

/*  
  warn << "(abs_diff <= bit_margin): "
       << "(" << abs_diff << " <= " << bit_margin << ") = "
        << ok;
  warn << "val1: " << hex << val1;
  warn << "val2: " << hex << val2;
*/  

  //
  // Determine the topmost bit of the absolute diff.
  // This used to be the topmost bits of the diff in bitwise values, see Note 1.
  //
  //uint32_t diff = (val1 ^ val2);

  for (int bit = 0; bit < size; bit++) {
    uint32_t mask = 1 << (size - 1 - bit);

    if (abs_diff & mask) {
      offset = (size - 1 - bit);
      break;
    }
  }

  if (offset <= ignore_bit) {
    offset = -1;
  } else {
    if (ignore_bit != -1) {
      warn << "bit_diff(): (offset > ignore_bit): (" << offset << " > " << ignore_bit << ")";
    }
  }

  return offset;  
}


/**
 * @brief Return the lowest upper of 16 for val.
 */
int resize_16(int in_val, bool do_dump) {
  int val = in_val;

  if ((val & 0xf) != 0) {
    val = (val + 15) & ~0xf;

    if (do_dump) {
      warn << "resize_16(): size " << in_val << " passed in,  adjusting to multiple of 16"
           << " -> " << val;
    }
  }

  return val;
}

}  // namespace V3DLib
