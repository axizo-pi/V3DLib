#ifndef _V3DLIB_SUPPORT_PLATFORM_H
#define _V3DLIB_SUPPORT_PLATFORM_H
#include "VCType.h"
#include <string>
// poep

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
Tag tag();

std::string vc_type_str(VCType type);
int  size_regfile();
int  max_qpus();
int  gather_limit();


namespace hw {

VCType type();
std::string type_str();
bool vc4();
bool vc6();
bool vc7();

}  // namespace hw

namespace compile {

void start(VCType in_type);
VCType type();
bool for_vc4();
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


namespace running {

bool vc4();

} // namespace running


namespace unpack {

void set_type(VCType vc_type);
void done();

VCType type();
bool vc4();
bool vc6();
bool vc7();

} // namespace unpack


void use_main_memory(bool val);
bool use_main_memory();


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
