#include "SmallLiteral.h"
#include <stdio.h>
#include "Source/Expr.h"
#include "Support/basics.h"

namespace V3DLib {

using ::operator<<;  // C++ weirdness


/**
 * @brief Small literals are literals that fit in the small immediate field
 *        of the `vc4` instruction set.
 *
 * Note that this is currently `vc4` only. `v3d` has different small imm values for floats.
 * There is considerable overlap with `SmallInt`.
 */
namespace SmallLit {

const int NUM_SMALL_FLOATS = 16;
const float smallFloats[NUM_SMALL_FLOATS] = {
    1.0
  , 2.0
  , 4.0
  , 8.0
  , 16.0
  , 32.0
  , 64.0
  , 128.0
  , 0.00390625
  , 0.0078125
  , 0.015625
  , 0.03125
  , 0.0625
  , 0.125
  , 0.25
  , 0.5
};


int encode(int val) {
  if (val >= 0 && val <= 15)
    return val;
  else if (val >= -16 && val <= -1)
    return 32 + val;

  return -1;
}


int encode(float val) {
  if (val == 0.0)
    return 0;
  else {
    int index = -1;

    for (int i = 0; i < NUM_SMALL_FLOATS; i++)
      if (smallFloats[i] == val) {
        index = i;
        break;
      }

    if (index != -1)
      return 32 + index;
  }

  return -1;
}


/**
 * Encode a small literal according to Table 5 of the VideoCore-IV manual.
 * 
 * @return encoded lit value, -1 if expression cannot be encoded
 */
int encode(Expr const &e) {
  if (e.tag() == Expr::INT_LIT) { 
    return encode(e.intLit);
  } else if (e.tag() == Expr::FLOAT_LIT) {
    return encode(e.floatLit);
  }

  return -1;
}


/**
 * Decode a small literal.
 */
Word decode(int x) {
  Word w;

  if (x >= 32) {
    w.floatVal = smallFloats[x - 32];
  } else if (x >= 16) {
    w.intVal = x - 32;
  } else {
    w.intVal = x;
  }

  return w;
}


/**
 * @brief Check if given input is a valid small imm value.
 *
 * The **value** is checked, not the small imm index
 */
bool valid(Word w) {
  return (encode(w.intVal) != -1) || (encode(w.floatVal) != -1);
}

}  // namespace SmallLit
}  // namespace V3DLib
