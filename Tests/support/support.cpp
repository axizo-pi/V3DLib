#include "support.h"
#include <sys/time.h>
#include <sstream>
#include <algorithm>
#include <iterator>



double get_time() {
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double) t.tv_sec + ((double) t.tv_usec) * 1e-6;
}


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
        "\nExpected: 0x" << std::hex << expected[n] << std::dec 
                         << Instr(expected[n]).dump() <<
        "Received: 0x"   << std::hex << received[n] << std::dec
                         << Instr(received[n]).dump()
      );

      REQUIRE(Instr::compare_codes(expected[n], received[n]));
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
      printf("Skipping v3d tests with calls to driver\n");
      did_first = true;
    }
    return false;
  }

  return true;
}


#ifdef QPU_MODE
//  #pragma message "QPU mode enabled"
const char *SUDO = (V3DLib::Platform::run_vc4())? "sudo " : "";  // sudo needed for vc4
#else
const char *SUDO = "";
#endif


void make_test_dir() {
  std::string cmd = SUDO;
  cmd += "mkdir -p obj/test";

  REQUIRE(!system(cmd.c_str()));

  cmd  = SUDO;
  cmd += "chmod ugo+rw obj/test";
  REQUIRE(!system(cmd.c_str()));
}


namespace {

template<typename T>
std::string dump_array_template(T const &a, int size,  int linesize, bool as_int = false) {
  std::string str("<");

  for (int i = 0; i < (int) size; i++) {
    if (linesize != -1) {
      if (i % linesize == 0) {
        str << "\n";
      }
    }

    if (as_int) {
      str << (int) a[i] << ", " ;
    } else {
      str << a[i] << ", " ;
    }
  }


  str << ">";
  return str;
}

}  // anon namespace

/**
 * Show contents of main memory array
 */
void dump_array(float *a, int size,  int linesize) {
  auto str = dump_array_template(a, size, linesize);
  printf("%s\n", str.c_str());
}


std::string dump_array2(float *a, int size,  int linesize) {
  return dump_array_template(a, size, linesize);
}


/**
 * Show contents of SharedArray instance
 */
void dump_array(V3DLib::Float::Array const &a, int linesize) {
  auto str = dump_array_template(a, a.size(), linesize, no_fractions(a));
  printf("%s\n", str.c_str());
}


std::string dump_array2(V3DLib::Float::Array const &a, int linesize) {
  return dump_array_template(a, a.size(), linesize, no_fractions(a));
}


void dump_array(V3DLib::Int::Array const &a, int linesize) {
  auto str = dump_array_template(a, a.size(), linesize);
  printf("%s\n", str.c_str());
}


namespace {

// Source: https://stackoverflow.com/questions/216823/how-can-i-trim-a-stdstring

// Trim from the start (in place)
inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// Trim from the end (in place)
inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}


struct BothAre
{
    char c;
    BothAre(char r) : c(r) {}
    bool operator()(char l, char r) const
    {
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



