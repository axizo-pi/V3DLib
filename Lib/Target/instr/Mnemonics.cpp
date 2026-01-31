#include "Mnemonics.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "global/log.h"
#include <memory>

using namespace Log;

namespace V3DLib {

using namespace Target;

namespace {

Instr genInstr(Enum op, Reg dst, RegOrImm const &srcA, RegOrImm const &srcB) {
  dst.can_write(true);
  srcA.can_read(true);
  srcB.can_read(true);

  Instr instr(ALU);
  instr.ALU.op   = ALUOp(op);
  instr.ALU.srcA = srcA;
  instr.ALU.srcB = srcB;
  instr.dest(dst);

  return instr;
}


Instr genInstr(Enum op, Reg dst, RegOrImm const &src) {
  dst.can_write(true);

  Instr instr(ALU);
  instr.ALU.op   = ALUOp(op);
  instr.ALU.srcA = src;
  instr.ALU.srcA.can_read(true);
  instr.ALU.srcB = Target::instr::None;
  instr.dest(dst);

  return instr;
}


/**
 * SFU functions always write to ACC4
 * Also 2 NOP's required; TODO see this can be optimized
 */
Instr::List sfu_function(Reg dst, RegOrImm const &srcA, Reg const &sfu_reg, const char *label) {
  using namespace V3DLib::Target::instr;

  std::string cmt = "SFU function ";
  cmt << label;

  Instr nop;
  auto mov1 = mov(sfu_reg, srcA);
	assert(mov1.size() == 1);
	mov1.back().comment(cmt);

  Instr::List ret;
  ret << mov1
      << nop
      << nop
      << mov(dst, ACC4());

  return ret;
}

Reg const ACC0_(ACC, 0);
Reg const ACC4_(ACC, 4);


Instr _mov(Reg dst, RegOrImm const &src) {
	return genInstr(Enum::A_MOV, dst, src); //.comment("_mov");
}

}  // anon namespace

namespace Target {
namespace instr {

Reg const None(NONE, 0);


/*================================================================
 * Replace ACCs's with rf registers for vc7.
 *
 * ACC0 is often used as a temp register for vc4 and vc6.
 * This does not work any more for vc7, which does not have 
 * general purpose accumulators.
 *
 * ACC1, ACC2, ACC3 do not appear to be used anywhere, removed.
 *  - ACC3 is not used on the Target level, but implicit in several
 *    mathematical functions on v3d.
 *  - ACC4 and ACC5 have special purposes
 *
 * Just remember that every call generates a new variable.
 * If you need to reuse a given ACC instance, keep a reference.
 =================================================================*/

Reg ACC0() {
	if (Platform::compiling_for_vc7()) {
		assert(V3DLib::VarGen::count() != 0);
	 	return Reg(V3DLib::VarGen::fresh());
	} else {
		return ACC0_;
	}
}


Reg _64() {
 	return Reg(Var_64());
}


/**
 * This is a special purpose register  on vc4, vc6.
 * It is not on vc7.
 */
Reg ACC4() {
	if (Platform::compiling_for_vc7()) {
		assert(V3DLib::VarGen::count() != 0);
	 	return Reg(V3DLib::VarGen::fresh());
	} else {
		return ACC4_;
	}
}


/**
 * This is a special purpose register on vc4, vc6.
 * It is still present vc7, but renamed to QUAD.
 */
Reg const ACC5(ACC, 5);

Reg const UNIFORM_READ( SPECIAL, SPECIAL_UNIFORM);
Reg const QPU_ID(       SPECIAL, SPECIAL_QPU_NUM);
Reg const ELEM_ID(      SPECIAL, SPECIAL_ELEM_NUM);
Reg const TMUA(SPECIAL, SPECIAL_TMUA);
Reg const VPM_WRITE(    SPECIAL, SPECIAL_VPM_WRITE);
Reg const VPM_READ(     SPECIAL, SPECIAL_VPM_READ);
Reg const WR_SETUP(     SPECIAL, SPECIAL_WR_SETUP);
Reg const RD_SETUP(     SPECIAL, SPECIAL_RD_SETUP);
Reg const DMA_LD_WAIT(  SPECIAL, SPECIAL_DMA_LD_WAIT);
Reg const DMA_ST_WAIT(  SPECIAL, SPECIAL_DMA_ST_WAIT);
Reg const DMA_LD_ADDR(  SPECIAL, SPECIAL_DMA_LD_ADDR);
Reg const DMA_ST_ADDR(  SPECIAL, SPECIAL_DMA_ST_ADDR);
Reg const SFU_RECIP(    SPECIAL, SPECIAL_SFU_RECIP);
Reg const SFU_RECIPSQRT(SPECIAL, SPECIAL_SFU_RECIPSQRT);
Reg const SFU_EXP(      SPECIAL, SPECIAL_SFU_EXP);
Reg const SFU_LOG(      SPECIAL, SPECIAL_SFU_LOG);

// Synonyms for v3d
Reg const TMUD(SPECIAL, SPECIAL_VPM_WRITE);


/////////////////////////////////////////////////////////////
// Perhaps needed for vc7, known in vc6, unknown for vc4
/////////////////////////////////////////////////////////////

/***********************************************************************************************************
 * Added to examine reads from uniform ptr's on vc7.
 * Here are notes from this examination:
 *
 * TODO: examine if these should be added for first call only
 *
 * Order important: TMUC needs to be after TMUAU to make a difference
 *
 * - TMUAU: 
 *     Appears to add result to src on output
 *     The src param does not appear to do anything. It matters that TMUAU is written to
 * - TMUC:
 * 		It looks like TMUC performs some kind of rotate on output; It doesn't appear to be consistent though.
 *    Also, the value appears to carry over to subsequent kernel calls; so global?
 *
 *    Trying out various values for param to TMUC:
 *
 *    Small imm:
 *      * 0..15  : no effect, no output written
 *      * 16     : encoding value fails
 *      * ~0     : result added by original value
 *      * -1..-3 : Same as ~0; takes some time for output to stabilize
 *      * -4     : Variable output, something happens. Weird addition combined with output rotate?
 *      * -5     : Idem previous, results different
 *      * -15,-16: no effect, no output written
 *      * -17    : encoding value fails
 *
 * --------------------------------------------------------
 *
 * *NOTE* : It might be that I did this wrong. If I want
 *          to be consistent about it, I will need to redo
 *          with thrsw and 2 NOP's on mov(TMUA,src).
 ***********************************************************************************************************/

Reg const TMUAU(        SPECIAL, SPECIAL_TMUAU);
Reg const TMUC(         SPECIAL, SPECIAL_TMUC);
Reg const TMUL(         SPECIAL, SPECIAL_TMUL);

// End perhaps needed for vc7

Reg rf(uint8_t index) {
  return Reg(REG_A, index);
}


Instr::List mov(Reg dst, RegOrImm const &src) {
  dst.can_write(true);

	Instr::List ret;

	//Log::debug << "dst: " << dst.dump() << "; " << "tag: " << dst.tag;   
	//Log::warn << "mov src: " << src.dump();   

	if (src.is_reg() && src.reg().tag == SPECIAL) {
		// The logic for special reg's is under bor(), so we 
		// need to redirect there
    ret <<  bor(dst, src, src);
	} else if (Platform::compiling_for_vc7()) {
    ret <<  _mov(dst, src);
	} else if (src.is_imm()) {
    ret << li(dst, src.imm());
  } else {
    ret << bor(dst, src, src);
  }

	return ret;
}


// Generation of bitwise instructions
Instr bor(Reg dst, RegOrImm const &srcA, RegOrImm const &srcB)  { return genInstr(Enum::A_BOR, dst, srcA, srcB); }
Instr bor(Reg dst, RegOrImm const &src)   { return genInstr(Enum::A_BOR, dst, src, src); }
Instr band(Reg dst, Reg srcA, Reg srcB)   { return genInstr(Enum::A_BAND, dst, srcA, srcB); }
Instr band(Reg dst, Reg srcA, int n)      { return genInstr(Enum::A_BAND, dst, srcA, n); }
Instr bxor(Var dst, RegOrImm srcA, int n) { return genInstr(Enum::A_BXOR, dst, srcA, n); }


/**
 * Generate left-shift instruction.
 */
Instr shl(Reg dst, Reg srcA, int val) {
  assert(val >= 0 && val <= 15);
  return genInstr(Enum::A_SHL, dst, srcA, val);
}


Instr shl(Reg dst, Reg srcA, Reg srcB) {
  return genInstr(Enum::A_SHL, dst, srcA, srcB);
}


Instr shr(Reg dst, Reg srcA, int n) {
  assert(n >= 0 && n <= 15);
  return genInstr(Enum::A_SHR, dst, srcA, n);
}


/**
 * Generate addition instruction.
 */
Instr add(Reg dst, Reg srcA, Reg srcB) {
	return genInstr(Enum::A_ADD, dst, srcA, srcB);
}


Instr add(Reg dst, Reg srcA, int n) {
  assert(n >= 0 && n <= 15);
  return genInstr(Enum::A_ADD, dst, srcA, n);
}


Instr sub(Reg dst, Reg srcA, int n) {
  assert(n >= 0 && n <= 15);
  return genInstr(Enum::A_SUB, dst, srcA, n);
}


Instr sub(Reg dst, int n, Reg srcA) {
  assert(n >= 0 && n <= 15);
  return genInstr(Enum::A_SUB, dst, n, srcA);
}


Instr fmul(Reg dst, RegOrImm const &srcA, RegOrImm const &srcB) {
  return genInstr(Enum::M_FMUL, dst, srcA, srcB);
}


Instr fsub(Reg dst, RegOrImm const &srcA, RegOrImm const &srcB) {
  return genInstr(Enum::A_FSUB, dst, srcA, srcB);
}


/**
 * Generate load-immediate instruction.
 */
Instr li(Reg dst, Imm const &src) {
  dst.can_write(true);

  //Log::warn << "li src: " << src.dump() << ", is float: " << src.is_float();

  Instr instr(LI);
  instr.LI.imm  = src;
  instr.dest(dst);
 
  return instr;
}


Instr itof(Reg dst, RegOrImm const &src) { return genInstr(Enum::A_ItoF, dst, src); }


/**
 * Create an unconditional branch.
 *
 * Conditions can still be specified with helper methods (e.g. see `allzc()`)
 */
Instr branch(Label label) {
  Instr instr(BRL);
  instr.branch_label(label);
  return instr;
}


/**
 * Target-level definition of a `barrier` instruction.
 *
 * The goal of `barrier` is for multi-QPU programs:
 * If one QPU reaches it, it waits for all other QPU's to get here.
 *
 * When all QPU's have reached it, execution of all QPU's continues.
 *
 * `vc4` has no barrier instruction; the best implementation is probably
 * by using semaphores (which don't exist on `v3d`).
 */
Instr barrier() {
  return Instr(BARRIER);
}


Instr::List recipsqrt(Var dst, Var srcA) {
	if (Platform::compiling_for_vc7()) {
		Instr::List ret;

 		// We also have Enum::A_RSQRT2d
 		ret << genInstr(Enum::A_RSQRT, dst, srcA);

		return ret;
	} else {
		return sfu_function(dst, srcA, SFU_RECIPSQRT, "recipsqrt");
	}
}


/**
 * This returns the log2 of the given value
 */
Instr::List blog(Reg dst, RegOrImm const &srcA) {
	if (Platform::compiling_for_vc7()) {
		Instr::List ret;
 		ret << genInstr(Enum::A_LOG, dst, srcA);
		return ret;
	} else {
		return sfu_function(dst, srcA, SFU_LOG, "log");
	}
}


Instr::List recip(Reg dst, RegOrImm const &srcA) {
	if (Platform::compiling_for_vc7()) {
		Instr::List ret;
  	ret << genInstr(Enum::A_RECIP, dst, srcA);
		return ret;
	} else {
		return sfu_function(dst, srcA, SFU_RECIP    , "recip");
	}
}


/**
 * This returns 2 to the power of srA.
 */
Instr::List bexp(Var dst, RegOrImm const &srcA) {
	if (Platform::compiling_for_vc7()) {
		Instr::List ret;
 		ret << genInstr(Enum::A_EXP, dst, srcA);
		return ret;
	} else {
		return sfu_function(dst, srcA, SFU_EXP      , "exp");
	}
}


Instr::List bexp_e(Var dst, RegOrImm const &srcA) {
	const float e_const = 2.71828f;

	Imm e(e_const);
	Reg tmp(V3DLib::VarGen::fresh());

	Instr::List ret;
	ret << blog(tmp, e);
	ret << fmul(tmp, srcA, tmp)
	    << bexp(dst, tmp);

	return ret;
}


/**
 * Create label instruction.
 *
 * This is a meta-instruction for Target source.
 */
Instr label(Label in_label) {
  Instr instr;
  instr.tag = LAB;
  instr.label(in_label);

  return instr;
}


/**
 * Load next value from TMU
 */
Instr recv(Reg dst) {
  Instr instr(RECV);
  instr.dest(dst);
  return instr;
}


/**
 * v3d only
 */
Instr tmuwt() {
  return genInstr(Enum::A_TMUWT, None, None, None);
}



}  // namespace instr
}  // namespace Target
}  // namespace V3DLib

