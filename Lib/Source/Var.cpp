#include "Var.h"
#include "Support/basics.h"
#include "Support/Platform.h"

/**
 * /file
 * Class Var
 */

namespace V3DLib {
namespace {

int globalVarId = 0;  // Used for fresh variable generation

}  // anon namespace


Var::Var(VarTag tag, bool is_uniform_ptr) : m_tag(tag), m_is_uniform_ptr(is_uniform_ptr) {
  assert(!is_uniform_ptr || tag == UNIFORM);
}


bool Var::is_uniform_ptr() const { return m_is_uniform_ptr; }


std::string Var::dump() const {
  std::string ret;
  bool found_it = true;

  switch(m_tag) {
    case STANDARD:       ret << "v" << m_id;    break;

    case UNIFORM:
      ret << "Uniform";
      if (m_is_uniform_ptr) {
        ret << " Ptr";
      }
    break;

    case QPU_NUM:       ret << "QPU_NUM";       break;
    case ELEM_NUM:      ret << "ELEM_NUM";      break;
    case VPM_READ:      ret << "VPM_READ";      break;
    case VPM_WRITE:     ret << "VPM_WRITE";     break;
    case MUTEX_ACQUIRE: ret << "MUTEX_ACQUIRE"; break;
    case MUTEX_RELEASE: ret << "MUTEX_RELEASE"; break;
    case TMU0_ADDR:     ret << "TMU0_ADDR";     break;
    case DUMMY:         ret << "Dummy";         break;

    default:            found_it = false;       break;
  }

  assert(found_it);

  return ret;
}


/**
 * @brief Obtain a fresh variable
 *
 * @return a new standard variable
 */
Var VarGen::fresh(VarTag tag) {
  return Var(tag, globalVarId++);
}

int VarGen::fresh_tag() {
  return globalVarId++;
}


/**
 * @brief Returns number of fresh vars used
 */
int VarGen::count() {
  return globalVarId;
}


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
  }

  return Var(STANDARD, tag);
}

} // anon namespace


/**
 * Reset fresh variable generator
 */
void VarGen::reset(int val) {
  assert(val >= 0);
  //warn << "VarGen::reset() val: " << val;
  globalVarId = val;

  // Reset the global var's before each new kernel compile
  tag_64 = -1;
  tag_NaN = -1;
  tag_Inf = -1;
}


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

}  // namespace V3DLib
