#include "GlobalConstants.h"
#include "Lang.h"
#include "StmtStack.h"
#include "Support/basics.h"

namespace V3DLib {

using ::operator<<; // C++ weirdness

namespace {

int tag_64 = -1;
int tag_NaN = -1;
int tag_Inf = -1;

/**
 * @brief define a specific global variable.
 *
 * The variable is defined **once** in the initialization step on a kernel.
 *
 * Works great on `v3d`, on `vc4` mostly and that's not good enough.
 * `vc4` doesn't need it anyway because initialization of a constant is a single operation.
 */
Var global_var(int &tag) {
  //warn << "Called global_var()";

  if (Platform::compiling_for_vc4()) {
    cerr << "Don't use global var's on vc4" << thrw;
  }

  if (tag == -1) {
    tag = V3DLib::VarGen::fresh_tag();
  //} else {
  //  warn << "global_var() tag already defined, not overwriting: " << tag;
  }

  return Var(STANDARD, tag);
}

} // anon namespace

/**
 * @brief Support for Global Constants.
 *
 * This is used for commonly occuring constant values in the librar,
 * which may occur often in the code. Notable examples are `Nan` and `Inf`.
 *
 * The constant are initialized _once_ on kernel initialization and can be used
 * multiple times. This saves on the initialization step, which can be extensive,
 * consisting of many operations.
 *
 * This is a consideration for `v3d`. On `vc4` this is not much of a consideration,
 * because it has a  `load imm 32` operation. Constant initialization is thus a
 * single operation for `vc4`.
 *
 * An alternative to `GlobalConstants` is to load constant values as uniforms, which
 * also saves on the constant initialization step. @see UniformConstants.
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

    if (tag_Inf != -1) {
      tmp.as_float(0x7f800000);  comment("Bit-value for Inf");
      Inf() = tmp;
      buf << "Inf, ";
    }

    if (tag_NaN != -1) {
      tmp.as_float(0x7f800001);  comment("Bit-value for NaN");
      NaN() = tmp;
      buf << "NaN, ";
    }

    if (!buf.empty()) {
      info << "Called GlobalConstants::init() initialized: " << buf;
    }
  });

  // It is entirely possible that no global constant are init'ed here,
  // If these are not used in a kernel.
  if (!src.empty()) {
    auto first = *src.begin();
    first->comment("Start init global constants");
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


Int   _64() { return Int  (Var_64()); }
Float NaN() { return Float(Var_NaN()); }
Float Inf() { return Float(Var_Inf()); }

}  // namespace V3DLib
