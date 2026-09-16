#include "GlobalConstants.h"
#include "Lang.h"
#include "StmtStack.h"
#include "Support/basics.h"
#include <map>

namespace V3DLib {

using ::operator<<; // C++ weirdness

namespace {

int tag_64       = -1;
int tag_NaN      = -1;
int tag_Inf      = -1;
int tag_MinInf   = -1;
int tag_MinFloat = -1;
int tag_MaxFloat = -1;

std::map<float, int> values;


/**
 * @brief define a specific global variable.
 *
 * The variable is defined **once** in the initialization step on a kernel.
 *
 * Works great on `v3d`, on `vc4` mostly and that's not good enough.
 * `vc4` doesn't need it anyway because initialization of a constant is a single operation.
 */
Var global_var(int &tag) {
  if (tag == -1) {
    tag = V3DLib::VarGen::fresh_tag();
  }

  return Var(STANDARD, tag);
}


Var global_value(float val) {
  //warn << "Called global_value: " << val;
  int tag = -1;

  auto it = values.find(val);
  if (it == values.end()) {
    tag = V3DLib::VarGen::fresh_tag();
    values.insert({val, tag});
  } else {
    tag = it->second;
  }

  return Var(STANDARD, tag);
}

} // anon namespace


/**
 * @brief Support for Global Constants.
 *
 * This is used for commonly occuring constant values in the library,
 * which may occur often in the code. Notable examples are `Nan` and `Inf`.
 *
 * The constant are initialized _once_ on kernel initialization and can be used
 * multiple times. This saves on the initialization step, which can be extensive,
 * consisting of many operations.
 *
 * This is a consideration for `v3d`.
 * On `vc4` this is easier, because it has operation `load imm 32`.
 * Constant initialization is thus a single operation for `vc4`.
 * Despite this, global constants are also used for `vc`.
 *
 * An alternative to `GlobalConstants` is to load constant values as uniforms, which
 * also saves on the constant initialization step. @see UniformConstants.
 *
 * =============================================================
 * Notes
 * -----
 *
 * - Interestingly, following didn't work as expected:
 *
 *     Float x = Inf();
 *
 *   `Inf()` returns a Float, and the result of this statement is that `x` becomes the same Float, i.e. not a copy.
 *   In essence, `x` is an alias of the returned value (more precisely, an alias of the underlying `Var` of the `Float`).
 *
 *   Much confusion ensued, but I am one step closer to enlightment.
 *   TODO: Can this be fixed?
 */
namespace GlobalConstants {

/**
 * @brief Reset the global var's before each new kernel compile
 */
void reset() {
  //info << "Called GlobalConstants::reset()";

  tag_64 = -1;
  tag_NaN = -1;
  tag_Inf = -1;
  tag_MinInf = -1;
  tag_MinFloat = -1;
  tag_MaxFloat = -1;

  values.clear();
}


void init(Stmt::Array &src) {
  assert(src.empty());

  src = tempStmt([] () {
    std::string buf;
    Float tmp;

    if (tag_64 != -1) {
      _64() = 64;                comment("Bit-value for 64");
      buf << "_64, ";
    }

    if (tag_NaN != -1) {
      tmp.as_float(0x7f800001);  comment("Bit-value for NaN");
      NaN() = tmp;
      buf << "NaN, ";
    }

    if (tag_Inf != -1) {
      tmp.as_float(0x7f800000);  comment("Bit-value for Inf");
      Inf() = tmp;
      buf << "Inf, ";
    }

    if (tag_MinInf != -1) {
      tmp.as_float(0xff800000);  comment("Bit-value for Minus Inf");
      MinInf() = tmp;
      buf << "MinInf, ";
    }

    if (tag_MinFloat != -1) {
      tmp.as_float(0xff7fffff);  comment("Bit-value for largest negative Float");
      MinFloat() = tmp;
      buf << "MinFloat, ";
    }

    if (tag_MaxFloat != -1) {
      tmp.as_float(0x7f7fffff);  comment("Bit-value for largest positive Float");
      MaxFloat() = tmp;
      buf << "MinFloat, ";
    }

    for (auto it = values.begin(); it != values.end(); ++it) {
      float val = it->first;

      std::string cmt = "Const float ";
      cmt  << val;

      tmp = val; comment(cmt);
      GlobalConst(val) = tmp;

      buf << cmt << ", ";
    }

    if (!buf.empty()) {
      info << "Called GlobalConstants::init() initialized: " << buf;
    }
  });

  // It is entirely possible that no global constant are init'ed here,
  // If these are not used in a kernel.
  if (!src.empty()) {
    auto first = *src.begin();
    first->sub_header("Start init global constants");
  }
}

}  // namespace GlobalConstants


/**
 * @brief Define a single global variable that contains the value 64.
 *
 * This is used mainly for incrementing pointers.
 * See also the `_64` register.
 */
Var Var_64() { return global_var(tag_64); }


/**
 * @brief Define a single global variable that contains the value NaN.
 *
 * See also the `_NaN` reg.
 */
Var Var_NaN() { return global_var(tag_NaN); }


/**
 * @brief Define a single global variable that contains the value Inf.
 *
 * See also the `_Inf` register.
 */
Var Var_Inf() { return global_var(tag_Inf); }

Var Var_MinInf()   { return global_var(tag_MinInf); }
Var Var_MinFloat() { return global_var(tag_MinFloat); }
Var Var_MaxFloat() { return global_var(tag_MaxFloat); }


Int   _64()      { return Int  (Var_64()); }
Float NaN()      { return Float(Var_NaN()); }

Float Inf()      {
  auto ret = Float(Var_Inf());
  //warn << "Called Float Inf() tag: " << tag_Inf << ", var: " << Var_Inf().dump() << ", ret: " << ret.dump();
  return ret;
}

Float MinInf()   { return Float(Var_MinInf()); }
Float MinFloat() { return Float(Var_MinFloat()); }
Float MaxFloat() { return Float(Var_MaxFloat()); }

Float GlobalConst(float val) {
  return Float(global_value(val));
}

}  // namespace V3DLib
