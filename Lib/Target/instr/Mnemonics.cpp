#include "Mnemonics.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "global/log.h"

namespace V3DLib {
namespace {

Instr genInstr(ALUOp::Enum op, Reg dst, RegOrImm const &srcA, RegOrImm const &srcB) {
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


Instr genInstr(ALUOp::Enum op, Reg dst, RegOrImm const &src) {
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
Instr::List sfu_function(Var dst, Var srcA, Reg const &sfu_reg, const char *label) {
  using namespace V3DLib::Target::instr;

  std::string cmt = "SFU function ";
  cmt << label;

  Instr nop;

  Instr::List ret;
  ret << mov(sfu_reg, srcA).comment(cmt)
      << nop
      << nop
      << mov(dst, ACC4());

  return ret;
}

Reg const ACC0_(ACC, 0);
Reg const ACC4_(ACC, 4);

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
Reg const TMU0_S(       SPECIAL, SPECIAL_TMU0_S);
Reg const TMUAU(        SPECIAL, SPECIAL_TMUAU);      // Perhaps needed for vc7, known in vc6, unknown for vc4
Reg const TMUC(         SPECIAL, SPECIAL_TMUC);       // idem
Reg const TMUL(         SPECIAL, SPECIAL_TMUL);       // idem
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
Reg const TMUA(SPECIAL, SPECIAL_DMA_ST_ADDR);

Reg rf(uint8_t index) {
  return Reg(REG_A, index);
}


Instr _mov(Reg dst, RegOrImm const &src) {
	return genInstr(ALUOp::A_MOV, dst, src);
}


Instr mov(Reg dst, RegOrImm const &src) {
  dst.can_write(true);

	//Log::debug << "dst: " << dst.dump() << "; " << "tag: " << dst.tag;   

	if (src.is_reg() && src.reg().tag == SPECIAL) {
		// The logic for special reg's is under bor(), so we 
		// need to redirect there
    return bor(dst, src, src);
	} else if (Platform::compiling_for_vc7()) {
    return _mov(dst, src);
	} else if (src.is_imm()) {
    return li(dst, src.imm().val);
  } else {
    return bor(dst, src, src);
  }
}



// Generation of bitwise instructions
Instr bor(Reg dst, RegOrImm const &srcA, RegOrImm const &srcB)  { return genInstr(ALUOp::A_BOR, dst, srcA, srcB); }
Instr band(Reg dst, Reg srcA, Reg srcB)   { return genInstr(ALUOp::A_BAND, dst, srcA, srcB); }
Instr band(Reg dst, Reg srcA, int n)      { return genInstr(ALUOp::A_BAND, dst, srcA, n); }
Instr bxor(Var dst, RegOrImm srcA, int n) { return genInstr(ALUOp::A_BXOR, dst, srcA, n); }


/**
 * Generate left-shift instruction.
 */
Instr shl(Reg dst, Reg srcA, int val) {
  assert(val >= 0 && val <= 15);
  return genInstr(ALUOp::A_SHL, dst, srcA, val);
}


Instr shr(Reg dst, Reg srcA, int n) {
  assert(n >= 0 && n <= 15);
  return genInstr(ALUOp::A_SHR, dst, srcA, n);
}


/**
 * Generate addition instruction.
 */
Instr add(Reg dst, Reg srcA, Reg srcB) {
  return genInstr(ALUOp::A_ADD, dst, srcA, srcB);
}


Instr add(Reg dst, Reg srcA, int n) {
  assert(n >= 0 && n <= 15);
  return genInstr(ALUOp::A_ADD, dst, srcA, n);
}


Instr sub(Reg dst, Reg srcA, int n) {
  assert(n >= 0 && n <= 15);
  return genInstr(ALUOp::A_SUB, dst, srcA, n);
}


/**
 * Generate load-immediate instruction.
 */
Instr li(Reg dst, Imm const &src) {
  dst.can_write(true);

  Instr instr(LI);
  instr.LI.imm  = src;
  instr.dest(dst);
 
  return instr;
}


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


Instr::List recip(Var dst, Var srcA)     { return sfu_function(dst, srcA, SFU_RECIP    , "recip"); }
Instr::List recipsqrt(Var dst, Var srcA) { return sfu_function(dst, srcA, SFU_RECIPSQRT, "recipsqrt"); }
Instr::List bexp(Var dst, Var srcA)      { return sfu_function(dst, srcA, SFU_EXP      , "exp"); }
Instr::List blog(Var dst, Var srcA)      { return sfu_function(dst, srcA, SFU_LOG      , "log"); }


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
  return genInstr(ALUOp::A_TMUWT, None, None, None);
}

}  // namespace instr
}  // namespace Target
}  // namespace V3DLib

