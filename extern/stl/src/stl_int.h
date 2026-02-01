#ifndef _STL_LIB_INTERFACE_H_
#define _STL_LIB_INTERFACE_H_
#include "stl.h"

namespace stl {

bool read(std::string const &filename, std::vector<openstl::Triangle> &triangles);
void write(std::string const &filename, std::vector<openstl::Triangle> const &triangles, bool do_binary);
std::string const dump(openstl::Triangle const &t);

} // namespace stl


#endif //  _STL_LIB_INTERFACE_H_
