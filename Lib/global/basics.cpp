#include "basics.h"
#include <sstream>


std::string &operator<<(std::string &a, float val) {
  // Output in scientific notation
  std::stringstream out;
  out << std::scientific;
  out << val;
  a += out.str();
  return a;
}


std::string &operator<<(std::string &a, double val) {
  // Output in scientific notation
  std::stringstream out;
  out << std::scientific;
  out << val;
  a += out.str();
  return a;
}


std::string int_hex(int val) {
  std::ostringstream ss;
  ss << "0x" << std::hex << val;
  return ss.str();
}

