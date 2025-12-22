/**
 * This is a trick worth remembering.
 *
 *  CASE(A_FADD) translates to:
 *
 *    case V3D_QPU_A_FADD: ret = "A_FADD"; break;
 */

#define CASE(l)  case V3D_QPU_##l: ret = #l; break;

std::string translate_add_op(enum v3d_qpu_add_op val) {
	std::string ret;

  switch (val) {
    CASE(A_FADD)
    CASE(A_FADDNF)
    // ...
  }

  assert(!ret.empty());
	if(ret.empty()) ret = "<<UNKNOWN>>";

  return ret;
}

#undef CASE

