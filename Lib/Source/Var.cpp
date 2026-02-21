#include "Var.h"
#include "Support/basics.h"
#include "Support/Platform.h"

using namespace Log;

namespace V3DLib {
namespace {

int globalVarId = 0;  // Used for fresh variable generation

}  // anon namespace


Var::Var(VarTag tag, bool is_uniform_ptr) : m_tag(tag), m_is_uniform_ptr(is_uniform_ptr)  {
  assert(!is_uniform_ptr || tag == UNIFORM);
}


bool Var::is_uniform_ptr() const { return m_is_uniform_ptr; }


std::string Var::dump() const {
  //warn << "Called Var dump(), tag: " << m_tag;

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
 * Obtain a fresh variable
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
 * Returns number of fresh vars used
 */
int VarGen::count() {
  return globalVarId;
}


namespace {

int tag_64 = -1;

} // anon namespace


/**
 * Reset fresh variable generator
 */
void VarGen::reset(int val) {
  assert(val >= 0);
  globalVarId = val;
  tag_64 = -1;  // Needs to be reset before each new kernel compile!
}


/**
 * Define a single global variable that contains the value 64.
 * This is used mainly for incrementing pointers.
 *
 * Works great on v3d, on vc4 mostly and that's nog good enough.
 *
 * See also the _64 reg.
 */
Var Var_64() {
  if (Platform::compiling_for_vc4()) {
    cerr << "Don't use the 64 global var on vc4" << thrw;
  }

  if (tag_64 == -1) {
    tag_64 = V3DLib::VarGen::fresh_tag();
  }

  return Var(STANDARD, tag_64);
}

}  // namespace V3DLib
