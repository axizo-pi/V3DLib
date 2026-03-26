#include "VPMRequest.h"

namespace V3DLib {

VPMLoadReq::VPMLoadReq(int setup) {
  assert((setup & 0xc0000000) == 0);

  numVecs = (setup >> 20) & 0xf;
  if (numVecs == 0) numVecs = 16;

  hor    = (setup >> 11) & 1;
  addr   = setup & 0xff;
  stride = (setup >> 12) & 0x3f;
  if (stride == 0) stride = 64;
}


std::string VPMLoadReq::dump() const {
  std::string ret;

  ret << "VPMLoadReq: (" 
      << "numVecs: " << numVecs << ", "
      << (hor?"hor":"vert")     << ", "
      << "addr: "    << addr    << ", "
      << "stride: "  << stride
      << ")"; 

  return ret;
}


VPMStoreReq::VPMStoreReq(int in_addr, bool in_hor, int in_stride) :
  addr(in_addr), hor(in_hor), stride(in_stride)
{}


VPMStoreReq::VPMStoreReq(int setup) {
  assert((setup & 0xc0000000) == 0);

  hor    = (setup >> 11) & 1;
  addr   = setup & 0xff;
  stride = (setup >> 12) & 0x3f;
  if (stride == 0) stride = 64;
}


int VPMStoreReq::code() const {
  assert(addr < 256);
  assert(stride >= 1 && stride <= 64); // Valid stride
  assert(hor == 0 || hor == 1); // Horizontal or vertical

  // Max values encoded as 0
  int tmp_stride = stride;
  if (tmp_stride == 64) tmp_stride = 0;
  
  // Setup code
  int ret = tmp_stride << 12;
  ret |= hor << 11;
  ret |= 2 << 8;

  if (addr != -1) {
    ret |= (addr & 0xff);
  }
  
  return ret;
}


std::string VPMStoreReq::dump() const {
  std::string ret;

  ret << "VPMStoreReq: (" 
      << (hor?"hor":"vert")    << ", "
      << "addr: "    << addr   << ", "
      << "stride: "  << stride
      << ")"; 

  return ret;
}


}  // namespace V3DLib
