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
  std::string ret;

  switch(m_tag) {
    case STANDARD:
      ret << "v" << m_id;
    break;
    case UNIFORM:
      ret << "Uniform";
      if (m_is_uniform_ptr) {
        ret << " Ptr";
      }
    break;
    case QPU_NUM:
      ret << "QPU_NUM";
    break;
    case ELEM_NUM:
      ret << "ELEM_NUM";
    break;
    case VPM_READ:
      ret << "VPM_READ";
    break;
    case VPM_WRITE:
      ret << "VPM_WRITE";
    break;
    case TMU0_ADDR:
      ret << "TMU0_ADDR";
    break;
    case DUMMY:
      ret << "Dummy";
    break;
    default:
      assert(false);
    break;
  }

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


/**
 * Reset fresh variable generator
 */
void VarGen::reset(int val) {
  assert(val >= 0);
  globalVarId = val;
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

	static bool did_it = false;
  static int tag = -1;
	if (!did_it) {
 		tag = V3DLib::VarGen::fresh_tag();
		did_it = true;
	}

 	return Var(STANDARD, tag);
}

}  // namespace V3DLib
