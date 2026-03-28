#ifndef _V3DLIB_SUPPORT_PLATFORM_H
#define _V3DLIB_SUPPORT_PLATFORM_H
#include <string>

namespace V3DLib {
namespace Platform {

enum Tag {
  not_pi,
  pi1,
  pi2,
  pi3,
  pi4,
  pi_zero,
  pi5
};


bool is_pi_platform();
std::string platform_info();
std::string pi_version();
bool run_vc4();
bool run_vc7();
Tag tag();
void compiling_for_vc4(bool val);
bool compiling_for_vc4();
bool compiling_for_vc7();
void use_main_memory(bool val);
bool use_main_memory();
int  size_regfile();
int  max_qpus();
int  gather_limit();
void running_emulator(bool val);
bool running_emulator();


class main_mem {
public:
  main_mem(bool val);
  ~main_mem();

private:
  bool m_prev;
};

}  // namespace Platform
}  // namespace V3DLib


#endif  // _V3DLIB_SUPPORT_PLATFORM_H
