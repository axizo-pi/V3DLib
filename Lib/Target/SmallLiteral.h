#ifndef _V3DLIB_SMALL_LITERAL_H_
#define _V3DLIB_SMALL_LITERAL_H_
#include <string>

namespace V3DLib {

class Expr;

/**
 * @brief Type for representing the values in a vector
 */
struct Word {

union {
  int32_t intVal = 0;
  float   floatVal; 
};

};

namespace SmallLit {

int encode(int val);
int encode(float val);
int encode(Expr const &e);
Word decode(int x);
bool valid(Word w);

}  // namespace SmallLit
}  // namespace V3DLib

#endif  // _V3DLIB_SMALL_LITERAL_H_
