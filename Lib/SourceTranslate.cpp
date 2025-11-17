#include "SourceTranslate.h"
#include <memory>
#include "Support/debug.h"
#include "global/log.h"
#include "Support/Platform.h"
#include "vc4/SourceTranslate.h"
#include "v3d/SourceTranslate.h"
#include "Target/instr/Mnemonics.h"

namespace {

std::unique_ptr<V3DLib::ISourceTranslate> _vc4_source_translate;
std::unique_ptr<V3DLib::ISourceTranslate> _v3d_source_translate;

}  // anon namespace


namespace V3DLib {

/**
 * Used by both v3d and vc4
 *
 * Compare with RECV handling in:
 *   - loadStorePass()
 *   - gather/receive
 */
Instr::List ISourceTranslate::load_var(Var &in_dst, Expr &e) {
  using namespace V3DLib::Target::instr;

  Instr::List ret;

  Reg src(e.deref_ptr()->var());
  Reg dst(in_dst);

	if (Platform::compiling_for_vc7()) {
		//BRAINFART: TODO cleanup
		//
		// TODO: examine if this should be added for first call only
		//
		// - TMUAU: Hypothesis: add result to src on output?
		// - TMUC:
		// 		It looks like TMUC performs some kind of rotate on output; It doesn't appear to be consistent though.
		//    Also, the value appears to carry over to subsequent kernel calls; so global?
		//
		// Order important: TMUC needs to be after TMUAU to make a difference
		//

		// The src param does not appear to do anything.
		// It matters that TMUAU is written to
		//debug("load_var(): adding TMUAU for vc7");
  	//ret << mov(TMUAU, src);
  	//ret << mov(TMUA, src);

		//debug("load_var(): adding TMUL for vc7");
  	//ret << mov(TMUL, src);

		//
		// Small imm:
	  //  * 0..15  : no effect, no output written
		//  * 16     : encoding value fails
		//  * ~0     : result added by original value
		//  * -1..-3 : Same as ~0; takes some time for output to stabilize
		//  * -4     : Variable output, something happens. Weird addition combined with output rotate?
		//  * -5     : Idem previous, results different
	  //  * -15,-16: no effect, no output written
		//  * -17    : encoding value fails
		//
		//debug("load_var(): adding TMUC for vc7");
  	//ret << mov(TMUC, ~0);
	}

  ret << mov(TMU0_S, src)
      << recv(dst);

	Log::debug << "\n" << ret.dump(true);

  return ret;
}

/**
 * Generate code to add an offset to the uniforms which are pointers.
 *
 * The calculated offset is assumed to be in ACC0 (vc4, vc6)
 */
Instr::List add_uniform_pointer_offset(Instr::List &code) {
  using namespace V3DLib::Target::instr;

	Reg acc = ACC0();

  Instr::List ret;

  // offset = 4 * vector_id;
  ret << mov(acc, ELEM_ID).comment("Initialize uniform ptr offsets")
      << shl(acc, acc, 2);

  // add the offset to all the uniform pointers
  for (int index = 0; index < code.size(); ++index) {
    auto &instr = code[index];

    if (!instr.isUniformLoad()) break;  // Assumption: uniform loads always at top

    if (instr.isUniformPtrLoad()) {
      ret << add(rf((uint8_t) index), rf((uint8_t) index), acc);
    }
  }

  ret.back().comment("End initialize uniform ptr offsets");

  return ret;
}


ISourceTranslate &getSourceTranslate() {
  if (Platform::compiling_for_vc4()) {
    if (_vc4_source_translate.get() == nullptr) {
      _vc4_source_translate.reset(new vc4::SourceTranslate());
    }
    return *_vc4_source_translate.get();
  } else {
    if (_v3d_source_translate.get() == nullptr) {
      _v3d_source_translate.reset(new v3d::SourceTranslate());
    }
    return *_v3d_source_translate.get();
  }
}

}  // namespace V3DLib
