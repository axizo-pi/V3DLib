#ifndef _V3DLIB_SUPPORT_PLATFORM_H
#define _V3DLIB_SUPPORT_PLATFORM_H
#include <string>

namespace V3DLib {
namespace Platform {

enum VCType {
  UNKNOWN,
  vc4,
  vc6,
  vc7
};

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

namespace compile {

void start(VCType in_type);
bool for_vc4(bool do_break = true);
bool for_vc7();
bool for_vc6();
bool running();
void done();

} // namespace compile


namespace emulate {

void start(VCType in_type);
VCType type();
bool running();
void done();

} // namespace emulate

void use_main_memory(bool val);
bool use_main_memory();
int  size_regfile();
int  max_qpus();
int  gather_limit();

VCType vc_type();
std::string vc_type_str(VCType type);
std::string vc_type_str();


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
