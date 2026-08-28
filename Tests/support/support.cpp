#include "support.h"
#include "Support/Helpers.h"
#include <algorithm>         // unique()
#include <filesystem>
#include <iterator>

namespace fs = std::filesystem;
using namespace V3DLib;


//
// get the base directory right for calling compiled apps.
//
#ifdef DEBUG
  #define POSTFIX_DEBUG "debug"
#else
  #define POSTFIX_DEBUG "release"
#endif

#define BIN_PATH  "obj/" POSTFIX_DEBUG "/bin"
#define TEST_PATH "obj/" POSTFIX_DEBUG "/test"


std::string bin_path()  { return BIN_PATH;  }
std::string test_path() { return TEST_PATH; }


/**
 * @param skip_nops  If true, don't compare nops in received output.
 *                   This indicates instructions which can't be generated to bytecode (yet)
 */
void match_kernel_outputs(
  std::vector<uint64_t> const &expected,
  std::vector<uint64_t> const &received,
  bool skip_nops) {
    using namespace V3DLib::v3d::instr;
    auto _nop = nop();

    // Arrays should eventually match exactly, including length
    // For now, just check progress
    auto len = expected.size();
    if (len > received.size()) {
      len = received.size();
    }

    // Outputs should match exactly
    for (uint32_t n = 0; n < len; ++n) {
      if (skip_nops && (_nop == received[n])) {
        //printf("nop at index %d\n", n);
        continue;
      }

      INFO("Comparing assembly index: " << n << ", code length: " << received.size() <<
        "\nExpected: 0x" << std::hex << expected[n] 
                         << " " << std::dec << Instr(expected[n]).dump() <<
        "\nReceived: 0x" << std::hex << received[n]
                         << " " << std::dec << Instr(received[n]).dump()
      );

      if(!Instr::compare_codes(expected[n], received[n])) {
        // There are different binary representationsi possible of a given instructions.
        // Double-check with string representation.
        Instr exp(expected[n]);
        Instr rec(received[n]);
        bool equal = (exp.dump() == rec.dump());
        REQUIRE(equal);
      }
    }
}


/**
 * Check if running on v4d
 *
 * Supplies a one-time warning if not.
 *
 * @param return true if running on v3d, false otherwise
 */
bool running_on_v3d() {
  static bool did_first = false;

  if (V3DLib::Platform::run_vc4()) {
    if (!did_first) {
      printf("Skipping v3d tests\n");
      did_first = true;
    }
    return false;
  }

  return true;
}


/**
 * TODO: rename to make_test_path()
 */
void make_test_dir() {
  bool ret = ensure_path_exists(test_path());
  REQUIRE(ret);
}


namespace {

template<typename T>
std::string dump_array_template(
  T const &a,
  int size,
  int linesize,
  bool as_int = false,
  bool do_first = false
) {
  assertq(linesize == -1 || linesize > 0, "linesize must be -1 or positive");

  std::string ret("<");

  auto output = [&a, as_int] (int i) -> std::string {
    std::ostringstream ret;  // Using streambuf simplifies float representation

    if (as_int) {
      // Force output to integer
      ret << (int) a[i] << ", " ;
    } else {
       ret << a[i] << ", " ;
    }

    return ret.str();
  };

  if (do_first) {
    assertq(linesize != -1, "do_first requires linesize");

    for (int i = 0; i < (int) size; i += linesize) {
      ret << output(i);
    }
  } else {
    for (int i = 0; i < (int) size; i++) {
      if (linesize != -1) {
        if (i % linesize == 0) {
          ret << "\n  ";
        }
      }
      ret << output(i);
    }
  }

  ret << ">";
  return ret;
}

}  // anon namespace


/**
 * Show contents of main memory array
 */
std::string dump_array(float *a, int size,  int linesize) {
  return dump_array_template(a, size, linesize);
}


std::string dump_array(std::vector<unsigned> const &a,  int linesize) {
  return dump_array_template(&a[0], (int) a.size(), linesize);
}


std::string dump_array(std::vector<float> const &a,  int linesize) {
  return dump_array_template(&a[0], (int) a.size(), linesize);
}


/**
 * Show contents of SharedArray instance
 */
std::string dump_array(V3DLib::Float::Array const &a, int linesize, bool do_first) {
  return dump_array_template(a, a.size(), linesize, no_fractions(a), do_first);
}


std::string dump_array(V3DLib::Int::Array const &a, int linesize) {
  return dump_array_template(a, a.size(), linesize);
}


namespace {

struct BothAre {
  char c;
  BothAre(char r) : c(r) {}
  bool operator()(char l, char r) const {
    return r == c && l == c;
  }
};

}  // anon namespace


/**
 * Replace multiple spaces with single space.
 *
 * Leading and trailing spaces are trimmed.
 *
 * Source: https://stackoverflow.com/questions/5561138/interview-question-trim-multiple-consecutive-spaces-from-a-string
 */
std::string condense_whitespace(std::string str) {
  std::string::iterator i = unique(str.begin(), str.end(), BothAre(' '));
  
  std::stringstream ss;
  std::copy(str.begin(), i, std::ostream_iterator<char>(ss /*std::cout*/, ""));

  auto ret = ss.str();
  trim(ret);
  return ret;
}



