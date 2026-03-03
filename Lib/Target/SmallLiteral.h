#ifndef _V3DLIB_SMALL_LITERAL_H_
#define _V3DLIB_SMALL_LITERAL_H_
#include <string>

namespace V3DLib {

class Expr;

/**
 * Type for representing the values in a vector
 */
union Word {
  int32_t intVal = 0;
  float floatVal; 
};


int encodeSmallInt(int val);
int encodeSmallFloat(float val);
int encodeSmallLit(Expr const &e);
std::string printSmallLit(int x);
Word decodeSmallLit(int x);

}  // namespace V3DLib

#endif  // _V3DLIB_SMALL_LITERAL_H_
