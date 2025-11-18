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
	}

  ret << mov(TMUA, src)
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


ISourceTranslate &vc4_SourceTranslate() {
	if (_vc4_source_translate.get() == nullptr) {
		_vc4_source_translate.reset(new vc4::SourceTranslate());
	}
	return *_vc4_source_translate.get();
}


ISourceTranslate &v3d_SourceTranslate() {
	if (_v3d_source_translate.get() == nullptr) {
		_v3d_source_translate.reset(new v3d::SourceTranslate());
	}
	return *_v3d_source_translate.get();
}


ISourceTranslate &getSourceTranslate() {
  if (Platform::compiling_for_vc4()) {
		return vc4_SourceTranslate();
  } else {
		return v3d_SourceTranslate();
  }
}

}  // namespace V3DLib
