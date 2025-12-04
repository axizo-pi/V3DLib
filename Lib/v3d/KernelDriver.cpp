#include "KernelDriver.h"
#include <iostream>
#include <memory>
#include "Driver.h"
#include "Source/Translate.h"
#include "Target/SmallLiteral.h"  // decodeSmallLit()
#include "Target/RemoveLabels.h"
#include "instr/Snippets.h"
#include "Support/basics.h"
#include "Support/Timer.h"
#include "SourceTranslate.h"
#include "instr/Encode.h"
#include "instr/Mnemonics.h"
#include "instr/OpItems.h"
#include "instr/BaseSource.h"
#include "global/log.h"
#include "Target/Satisfy.h"
#include "Combine.h"

namespace V3DLib {
namespace v3d {

using namespace V3DLib::v3d::instr;
using Instructions = V3DLib::v3d::Instructions;
using namespace Log;

namespace {

// WEIRDNESS, due to changes, this file did not compile because it suddenly couldn't find
// the relevant overload of operator <<.
// Adding this solved it. HUH??
// Also: the << definitions in `basics.h` DID get picked up; the std::string versions did not.
using ::operator<<; // C++ weirdness

std::vector<std::string> local_errors;


/**
 * For v3d, the QPU and ELEM num are not special registers but instructions.
 *
 * In order to not disturb the code translation too much, they are derived from the target instructions:
 *
 *    mov(ACC0, QPU_ID)   // vc4: QPU_NUM  or SPECIAL_QPU_NUM
 *    mov(ACC0, ELEM_ID)  // vc4: ELEM_NUM or SPECIAL_ELEM_NUM
 *
 * This is the **only** operation in which they can be used.
 * This function checks for proper usage.
 * These special cases get translated to `tidx(r0)` and `eidx(r0)` respectively, as a special case
 * for A_BOR.
 *
 * If the check fails, a fatal exception is thrown.
 *
 * ==================================================================================================
 *
 * * Both these instructions use r0 here; this might produce conflicts with other instructions
 *   I haven't found a decent way yet to compensate for this.
 */
void checkSpecialIndex(V3DLib::Instr const &src_instr) {
  if (src_instr.tag != ALU) {
    return;  // no problem here
  }

  auto srca = src_instr.ALU.srcA;
  auto srcb = src_instr.ALU.srcB;

  bool a_is_elem_num = (srca.is_reg() && srca.reg().tag == SPECIAL && srca.reg().regId == SPECIAL_ELEM_NUM);
  bool a_is_qpu_num  = (srca.is_reg() && srca.reg().tag == SPECIAL && srca.reg().regId == SPECIAL_QPU_NUM);
  bool b_is_elem_num = (srcb.is_reg() && srcb.reg().tag == SPECIAL && srcb.reg().regId == SPECIAL_ELEM_NUM);
  bool b_is_qpu_num  = (srcb.is_reg() && srcb.reg().tag == SPECIAL && srcb.reg().regId == SPECIAL_QPU_NUM);
  bool a_is_special  = a_is_elem_num || a_is_qpu_num;
  bool b_is_special  = b_is_elem_num || b_is_qpu_num;

  if (!a_is_special && !b_is_special) {
    return;  // Nothing to do
  }

  if (src_instr.ALU.op.value() != ALUOp::A_BOR && src_instr.ALU.op.value() != ALUOp::A_MOV) {  // A_MOV is vc7-specific
    // All other instructions disallowed
    fatal("For v3d, special registers QPU_NUM and ELEM_NUM can only be used in a move instruction");
    return;
  }

	if (srca.is_none() || srcb.is_none()) {
		return;
	}

	// Following probably outdated since None sources were introduced
  assertq((a_is_special && b_is_special), "src a and src b must both be special for QPU and ELEM nums");
  assertq(srca == srcb, "checkSpecialIndex(): src a and b must be the same if they are both special num's");
}


/**
 * Pre: `checkSpecialIndex()` has been called
 */
bool is_special_index(V3DLib::Instr const &src_instr, Special index ) {
  assert(index == SPECIAL_ELEM_NUM || index == SPECIAL_QPU_NUM);

  if (src_instr.tag != ALU) {
    return false;
  }

  if (src_instr.ALU.op.value() != ALUOp::A_BOR) {
    return false;
  }

  auto srca = src_instr.ALU.srcA;
  auto srcb = src_instr.ALU.srcB;
  bool a_is_special = (srca.is_reg() && srca.reg().tag == SPECIAL && srca.reg().regId == index);
  bool b_is_special = (srcb.is_reg() && srcb.reg().tag == SPECIAL && srcb.reg().regId == index);

  return (a_is_special && b_is_special);
}


bool handle_special_index(V3DLib::Instr const &src_instr, Instructions &ret) {
  if (src_instr.tag == ALU && src_instr.ALU.op == ALUOp::A_TMUWT) {
    ret << tmuwt();
    return true;
  }

  auto dst_reg = encodeDestReg(src_instr);
  assert(dst_reg);

  auto reg_a = src_instr.ALU.srcA;
  auto reg_b = src_instr.ALU.srcB;

  if (reg_a.is_reg() && reg_b.is_reg()) {
    checkSpecialIndex(src_instr);
    if (is_special_index(src_instr, SPECIAL_QPU_NUM)) {
      ret << tidx(*dst_reg);
      return true;
    } else if (is_special_index(src_instr, SPECIAL_ELEM_NUM)) {
      ret << eidx(*dst_reg);
      return true;
    }
  }

  return false;
}


bool translateOpcode(V3DLib::Instr const &src_instr, Instructions &ret) {
  if (handle_special_index(src_instr, ret)) return true;

  bool did_something = true;

  auto reg_a = src_instr.ALU.srcA;
  auto reg_b = src_instr.ALU.srcB;

  assertq(src_instr.ALU.op.value() != ALUOp::A_FSIN || (reg_a.is_reg() && reg_b.is_reg()), "sin has smallims");

  auto dst_reg = encodeDestReg(src_instr);
  assert(dst_reg);

  switch (src_instr.ALU.op.value()) {
  case ALUOp::A_FSIN:
    assert(src_instr.ALU.oneOperand());
    ret << fsin(*dst_reg, reg_a);
    break;
  case ALUOp::A_FFLOOR:
    assert(src_instr.ALU.oneOperand());
    ret << ffloor(*dst_reg, reg_a);
    break;
  case ALUOp::A_TMUWT:
    assert(src_instr.ALU.noOperands());
    ret << tmuwt();
    break;
  case ALUOp::A_TIDX:
    breakpoint  // Apparently never called?
    assert(src_instr.ALU.noOperands());
    ret << tidx(*dst_reg);
    break;
  case ALUOp::A_EIDX:
    assert(src_instr.ALU.noOperands());
    ret << eidx(*dst_reg);
    break;
  case ALUOp::A_MOV: {
    assert(src_instr.ALU.oneOperand());

    auto tmp = mov(*dst_reg, reg_a);

		// BRAINFART: TODO cleanup. This solution is totally disgusting
		if (*dst_reg == tmua) {
			// Following thrsw and nop's absolutely required on vc7, verified
			tmp.thrsw();
			ret << tmp << nop() << nop();
		} else {
			ret << tmp;
		}
	 }
   break;
  default: {
    // Handle general case
    Instr instr;

    if (instr.alu_set(src_instr)) {
      ret << instr;
    } else {
      did_something = false;
    }
  }
  }

  if (did_something) return true;

  auto const &src_alu = src_instr.ALU;
  std::string msg = "translateOpcode(): Unknown conversion for src ";
  msg  << "op: " << src_alu.op.value()
       << ", instr: " << src_instr.dump();
  assertq(false, msg, true);

  return false;
}


void handle_condition_tags(V3DLib::Instr const &src_instr, Instructions &ret) {
  using ::operator<<; // C++ weirdness

  auto cond = src_instr.assign_cond();

  // src_instr.ALU.cond.tag has 3 possible values: NEVER, ALWAYS, FLAG
  assertq(cond.tag != AssignCond::Tag::NEVER, "NEVER encountered in ALU.cond.tag", true);          // Not expecting it
  assertq(cond.tag == AssignCond::Tag::FLAG || cond.is_always(), "Really expecting FLAG here", true); // Pedantry

  auto setCond = src_instr.set_cond();

  if (!setCond.flags_set() && !ret.empty()) {
		// Only set final instruction! This is the assignment of the result of the  preceding operations
    ret.back().set_cond_tag(cond);

    //
    // Dump fails under some mysterious circumstances, even when flags not used.
    // Eg. when doing '*int_result = -1;' in kernels.
    //
    // It happens elswhere as well. Not fighting it.
    //
    //cdebug << "handle_condition_tags(): set_cond_tag: '"
    //       << "v3d: \n" << ret.dump() << "'\n";

    return;
  }

  //
  // Set a condition flag with current instruction
  //
  // The condition is only set for the last in the list.
  // Any preceding instructions are assumed to be for calculating the condition
  //
  assertq(cond.is_always(), "Currently expecting only ALWAYS here", true);

  bool is_final_where_cond = src_instr.comment().find("where condition final") != src_instr.comment().npos;

  if (!is_final_where_cond) {
    ret.back().set_push_tag(setCond);
    return;
  }

  //
  // Process final where condition
  // In this case, condition flag must be pushed for both add and mul alu.
  //

/*		
    warn << "handle_condition_tags(): detected final where condition: '"
           << src_instr.dump() << "'\n"
           << "v3d: " << ret.back().mnemonic() << "'\n";
*/					 

  ret.back().set_push_tag(setCond);

  //
  // Add and mul alus are set to have the same destination, normally this is risky!
  // However, in this case the dst is a dummy (assumption? check), so if the hardware
  // allows it, this is ok.
  //
  // or.pushz  r0, r3, r3 ; mov.pushz  r0, r3
  //
/*
  Instructions tmp;
  assertq(translateOpcode(src_instr, tmp), "translateOpcode() failed");
  assert(tmp.size() == 1);
  Instr &tmp_instr = tmp[0];
*/
/*
  Instr tmp_instr;
  if(!tmp_instr.alu_mul_set(src_instr)) {
    assert(false);
  }

  tmp_instr.set_push_tag(setCond);
*/


  Instr tmp_instr;
  auto reg = encodeDestReg(src_instr);
  assert(reg);
  tmp_instr = nop().sub(*reg, *reg, SmallImm(0)).pushz();

  ret << tmp_instr;

  if (false) {
    warn   << "handle_condition_tags() "
           << "v3d final: " << ret.back().mnemonic() << "'\n"
           << "v3d tmp: " << tmp_instr.mnemonic() << "'\n";
  }
}


/**
 * @return true if rotate handled, false otherwise
 *
 * ============================================================================
 * NOTES
 * =====
 *
 * * From vc4 reg doc (assuming also applies to v3d):
 *   - An instruction that does a vector rotate by r5 must not immediately follow an instruction that writes to r5.
 *   - An instruction that does a vector rotate must not immediately follow an instruction that writes
 *      to the accumulator that is being rotated.
 *
 *   This implies that the srcA is always an accumulator, srcB either smallimm or r5.
 *   Adding preceding NOP is too strict now, assuming r0 always, and in fact 2 NOPs are added.
 *   TODO make better.
 *
 * * A rotate is actually a mov() with the rotate signal set.
 * * on vc7 ROT is an operation on add.
 */
bool translateRotate(V3DLib::Instr const &instr, Instructions &ret) {
  if (!instr.ALU.op.isRot()) return false;

	auto dst_reg = encodeDestReg(instr);
  assert(dst_reg);

  auto reg_a = instr.ALU.srcA;
  auto src_a = encodeSrcReg(reg_a.reg());
  auto reg_b = instr.ALU.srcB;                  // reg b is either r5 or small imm


	if (Platform::compiling_for_vc7()) {
		// Assumptions
		//  - waddr is an rf register (logical)
		//  - Thing to rotate is in add a
		//  - rot value is a small imm in add a
		//  - nop not required after rotate
		cdebug << "translateRotate vc7 input instr: " << instr.dump();

		assert(dst_reg->is_rf());
		assert(src_a->is_rf());
		assert(reg_b.is_imm());  // rf not handled yet
	
		Instr dst_instr;
		dst_instr.alu.add.op = V3D_QPU_A_ROT;
  	dst_instr.alu_add_set(*dst_reg, *src_a, reg_b);

		cdebug << "translateRotate vc7 instruction: " << dst_instr.mnemonic(false);

		ret << dst_instr;

	} else {
  	// dest is location where r1 (result of rotate) must be stored 
	  assertq(dst_reg->to_mux() != V3D_QPU_MUX_R1, "Rotate can not have destination register r1", true);

	  if (src_a->to_mux() != V3D_QPU_MUX_R0) {
	    ret << mov(r0, *src_a).comment("moving param 2 of rotate to r0. WARNING: r0 might already be in use, check!");
	  }

	  // TODO: the Target source step already adds a nop.
	  //       With the addition of previous mov to r0, the 'other' nop becomes useless, remove that one for v3d.
	  ret << nop().comment("NOP required for rotate");

	  if (reg_b.is_reg()) {
	    breakpoint  // Not called yet

	    assert(instr.ALU.srcB.reg().tag == ACC && instr.ALU.srcB.reg().regId == 5);  // reg b must be r5
	    auto src_b = encodeSrcReg(reg_b.reg());

	    ret << rotate(r1, r0, *src_b);

	  } else {
	    ret << rotate(r1, r0, reg_b.imm());
	  }

	  ret << bor(*dst_reg, r1, r1);
	}

  return true;
}


Instructions encode_LI(V3DLib::Instr const full_instr) {
  assert(full_instr.tag == LI);
  auto &instr = full_instr.LI;
  auto dst = encodeDestReg(full_instr);

  //warn << "encode_LI instr: " << full_instr.dump() << "\n"
  //     << "   imm: " << full_instr.LI.imm.dump();

  Instructions ret;

  switch (instr.imm.tag()) {
  case Imm::IMM_INT32: {
    int value = instr.imm.intVal();
    ret << mov(*dst , Source(value));
  }
  break;

  case Imm::IMM_FLOAT32: {
    float value = instr.imm.floatVal();
    ret << mov(*dst , Source(value));
  }
  break;

  case Imm::IMM_MASK:
    debug_break("encode_LI(): IMM_MASK not handled");
  break;

  default:
    debug_break("encode_LI(): unknown tag value");
  break;
  }

  if (full_instr.set_cond().flags_set()) {
    breakpoint;  // to check what flags need to be set - case not handled yet
  }

  ret.set_cond_tag(full_instr.assign_cond());
  return ret;
}


Instructions encodeALUOp(V3DLib::Instr instr) {
  Instructions ret;

  if (instr.isUniformLoad()) {
    uint8_t rf_addr = to_waddr(instr.dest());
    ret << nop().ldunifrf(rf(rf_addr));
   } else if (translateRotate(instr, ret)) {
    handle_condition_tags(instr, ret);
  } else if (translateOpcode(instr, ret)) {
    handle_condition_tags(instr, ret);
  } else {
    assertq(false, "Missing translate operation for ALU instruction", true);  // Something missing, check
  }

  assert(!ret.empty());
  return ret;
}


/**
 * Create a branch instruction, including any branch conditions,
 * from Target source instruction.
 */
v3d::instr::Instr encodeBranchLabel(V3DLib::Instr src_instr) {
  assert(src_instr.tag == BRL);

  // Prepare as branch without offset but with label
  auto dst_instr = branch(0, true);
  dst_instr.label(src_instr.branch_label());
  dst_instr.set_branch_condition(src_instr.branch_cond());

  return dst_instr;
}


/**
 * Convert intermediate instruction into core instruction.
 *
 * **Pre:** All instructions not meant for v3d are detected beforehand and flagged as error.
 */
Instructions encodeInstr(V3DLib::Instr instr) {
  Instructions ret;

  // Encode core instruction
  switch (instr.tag) {
    //
    // Unhandled tags - ignored or should have been handled beforehand
    //
    case BR:
      assertq(false, "Not expecting BR any more, branch creation now goes with BRL", true);
    break;

    case INIT_BEGIN:
    case INIT_END:
    case END:         // vc4 end program marker
      assertq(false, "Not expecting INIT or END tag here", true);
    break;

    //
    // Label handling
    //
    case LAB: {
      // create a label meta-instruction
      Instr n;
      n.is_label(true);
      n.label(instr.label());
      
      ret << n;
    }
    break;

    case BRL: {
      // Create branch instruction with label
      ret << encodeBranchLabel(instr);
    }
    break;

    case RECV: {
      auto dst_reg = encodeDestReg(instr);
      assert(dst_reg);

      ret << nop().ldtmu(*dst_reg);
    }
    break;

    case LI:           ret << encode_LI(instr);   break;
    case ALU:          ret << encodeALUOp(instr);           break;
    case NO_OP:        ret << nop();                        break;

    default:
      fatal("v3d: missing case in encodeInstr");
  }

  assert(!ret.empty());

  if (!ret.empty()) {
    ret.front().transfer_comments(instr);
  }

  return ret;
}


/**
 * This is where standard initialization code can be added.
 *
 * Called;
 * - after code for loading uniforms has been encoded
 * - any other target initialization code has been added
 * - before the encoding of the main body.
 *
 * Serious consideration: Any regfile registers used in the generated code here,
 * have not participated in the liveness determination. This may lead to screwed up variable assignments.
 * **Keep this in mind!**
 */
Instructions encode_init() {
  Instructions ret;
  ret << instr::enable_tmu_read();

  return ret;
}


#ifdef DEBUG

/**
 * Check assumption: uniform loads are always at the top of the instruction list.
 */
bool checkUniformAtTop(V3DLib::Instr::List const &instrs) {
  bool doing_top = true;

  for (int i = 0; i < instrs.size(); i++) {
    V3DLib::Instr instr = instrs[i];
    if (doing_top) {
      if (instr.isUniformLoad()) {
        continue;  // as expected
      }

      doing_top = false;
    } else {
      if (!instr.isUniformLoad()) {
        continue;  // as expected
      }

      {
        std::string msg;
        msg << "checkUniformAtTop() failed at position " << i;
        warning(msg);
      }
      return false;  // Encountered uniform NOT at the top of the instruction list
    }
  }

  return true;
}

#endif  // DEBUG


/**
 * Translate instructions from target to v3d
 */
void _encode(V3DLib::Instr::List const &instrs, Instructions &instructions) {
#ifdef DEBUG	
  assertq(checkUniformAtTop(instrs), "_encode(): checkUniformAtTop() failed (v3d)", true);
#endif
  bool prev_was_init_begin = false;
  bool prev_was_init_end   = false;

  // Main loop
  for (int i = 0; i < instrs.size(); i++) {
    V3DLib::Instr instr = instrs[i];
    assertq(!instr.isZero(), "Zero instruction encountered", true);
    check_instruction_tag_for_platform(instr.tag, false);

    if (instr.tag == INIT_BEGIN) {
      prev_was_init_begin = true;
    } else if (instr.tag == INIT_END) {
      instructions << encode_init();
      prev_was_init_end = true;
    } else {
      Instructions ret = v3d::encodeInstr(instr);

      if (prev_was_init_begin) {
        ret.front().header("Init block");
        prev_was_init_begin = false;
      }

      if (prev_was_init_end) {
        ret.front().header("Main program");
        prev_was_init_end = false;
      }

      instructions << ret;
    }
  }

  instructions << sync_tmu()
               << end_program();
}


/**
 * NOTE: small imm not checked, perhaps this gets fixed downstream
 */
template<typename AddAlu>
bool can_be_mul_alu(AddAlu const &add_alu) {

	if (Platform::compiling_for_vc7()) {
		// Block write to special registers for now
		// Actually, this should be possible (on vc7 at least), as long as the special registers aren't the same
		// on add and mul. TODO examine
	  if (add_alu.magic_write) return false;

		// ORs with 1 source can be translated to mul alu MOV
	  if (add_alu.op == V3D_QPU_A_OR && add_alu.a.raddr == add_alu.b.raddr) return true;

	  return (
	    add_alu.op == V3D_QPU_A_ADD ||
	    add_alu.op == V3D_QPU_A_SUB ||
	    add_alu.op == V3D_QPU_A_MOV
		);
	} else {
		// Don't write to special registers in the mul alu
    if (add_alu.magic_write && add_alu.waddr >= V3D_QPU_WADDR_NOP) return false;

		// ORs with 1 source can be translated to mul alu MOV
	  if (add_alu.op == V3D_QPU_A_OR && add_alu.a.mux == add_alu.b.mux) return true;

	  return (
	    add_alu.op == V3D_QPU_A_ADD ||
	    add_alu.op == V3D_QPU_A_SUB
		);
	}
}


/**
 * Check if given instructions have a dependency on each other.
 *
 * Pre: Instruction 'first' is predecessor of 'second'.
 * There is a dependency if:
 *   - instruction 'second' has a source which is a destination of 'first'.
 *   - instruction 'second' has a destination which is a destination of 'first'.
 */
bool have_dependency(v3d::instr::Instr const &first, v3d::instr::Instr const &second) {
  return second.is_src(first.sig_dest())
      || second.is_src(first.add_dest())
      || second.is_src(first.mul_dest())
      || second.is_dst(first.sig_dest())
      || second.is_dst(first.add_dest())
      || second.is_dst(first.mul_dest());
}


bool can_combine(v3d::instr::Instr const &instr1, v3d::instr::Instr const &instr2, bool &do_converse) {
  assert(instr1.add_nop() || instr1.mul_nop());  // Not expecting fully filled instructions
  assert(instr2.add_nop() || instr2.mul_nop());  // idem

  // Skip branches
  if (instr1.type == V3D_QPU_INSTR_TYPE_BRANCH || instr2.type == V3D_QPU_INSTR_TYPE_BRANCH) return false;

  // Skip special signals for now - there might be something to be won with the ld's
  if (instr1.has_signal() || instr2.has_signal()) return false;

  // Skip full NOPs, they are there for a reason
  if (instr1.is_nop()) return false;
  if (instr2.is_nop()) return false;

  // skip both mul for now, needs extra logic and is probably scarce
  if (!instr1.mul_nop() && !instr2.mul_nop())  {
    return false;
  }


  auto magic_write1 = instr1.mul_nop()?instr1.alu.add.magic_write:instr1.alu.mul.magic_write;
  auto waddr1       = instr1.mul_nop()?instr1.alu.add.waddr:instr1.alu.mul.waddr;
  auto magic_write2 = instr2.mul_nop()?instr2.alu.add.magic_write:instr2.alu.mul.magic_write;
  auto waddr2       = instr2.mul_nop()?instr2.alu.add.waddr:instr2.alu.mul.waddr;

  // Skip combined special waddresses - important for tmu operations
  if ((magic_write1 && waddr1 >= V3D_QPU_WADDR_NOP)
   && (magic_write2 && waddr2 >= V3D_QPU_WADDR_NOP)) return false;

  // Disallow same dest reg
  if (waddr1 == waddr2 && magic_write1 == magic_write2) return false;


  // Don't combine set conditional with use conditional
  if (instr1.flags.apf && instr2.flags.ac) return false;


  // Output instr1 should not be used as input instr2
	if (Platform::compiling_for_vc7()) {
		// vc7 - no mux's, add/mul.a can also be small imm, raddr's in different location
		//cdebug << "can_combine() - vc7";

	  auto a2 = instr2.mul_nop()?instr2.alu.add.a:instr2.alu.mul.a;
	  auto b2 = instr2.mul_nop()?instr2.alu.add.b:instr2.alu.mul.b;

		bool small_imm_a = instr2.sig.small_imm_a == 1;
		bool small_imm_b = instr2.sig.small_imm_b == 1;

		if ( !( (!small_imm_a && !small_imm_b) || (small_imm_a != small_imm_b))) {
			cerr << "can_combine immediate check fails, instr2: " << instr2.mnemonic(false) << thrw;
		}
	
	  if (magic_write1) {
			return false;
	  } else {
	    if (!small_imm_a && a2.raddr == waddr1) return false;
	    if (!small_imm_b && b2.raddr == waddr1) return false;
	  }
	} else {
		// vc6 - same except for location mux's

		bool small_imm_b = instr2.sig.small_imm_b == 1;

		// Small imm can only be in add b
		if (small_imm_b) return false;

	  auto a2_mux = instr2.mul_nop()?instr2.alu.add.a.mux:instr2.alu.mul.a.mux;
	  auto b2_mux = instr2.mul_nop()?instr2.alu.add.b.mux:instr2.alu.mul.b.mux;
	
	  bool is_rf1 = !magic_write1;
	  if (is_rf1) {
	    if (a2_mux == V3D_QPU_MUX_A && instr2.raddr_a == waddr1) return false;
	    if (b2_mux == V3D_QPU_MUX_A && instr2.raddr_a == waddr1) return false;

	    if (a2_mux == V3D_QPU_MUX_B && !small_imm_b && instr2.raddr_b == waddr1) return false;
	    if (b2_mux == V3D_QPU_MUX_B && !small_imm_b && instr2.raddr_b == waddr1) return false;
	  } else {
	    if (a2_mux < V3D_QPU_MUX_A && a2_mux == waddr1) return false;
	    if (b2_mux < V3D_QPU_MUX_A && b2_mux == waddr1) return false;
	  }
	}

  // mul/alu splits can always be combined
  if (instr1.mul_nop() && !instr2.mul_nop()) {
    do_converse = false;
    return true;
  }

  if (!instr1.mul_nop() && instr2.mul_nop()) {
    do_converse = true;
    return true;
  }


  //
  // Determine add alu instructions with mul alu equivalents
  //
  if (can_be_mul_alu(instr2.alu.add)) {
    do_converse = false;
    return true;
  }

  if (can_be_mul_alu(instr1.alu.add)) {
    do_converse = true;
    return true;
  }

  return false;
}


void combine(Instructions &instructions) {
  int combine_count = 0;

  for (int i = 1; i < (int) instructions.size(); i++) {
    auto &instr1 = instructions[i - 1];
    auto &instr2 = instructions[i];

    assertq(!instr1.mnemonic(false).empty(), "WTF 1", true);  // Paranoia
    assertq(!instr2.mnemonic(false).empty(), "WTF 2", true);

    assertq(!(instr1.skip() && instr2.skip()), "Deal with skips when they happen");
    if (instr1.skip()) continue;

    //
    // Combine pure nop ldtmu instruction with next
    // 
    if (false && instr1.is_ldtmu() && instr1.is_nop()) { // TODO: check reason for false

      // There can be multiple consecutive tmu loads, shift them all in reverse order
      int first = i - 1;
      int last  = i - 1;
      for (int n = last + 1; n < (int) instructions.size(); n++) {
        auto &instr = instructions[n];

        assert(!instr.skip());
        if (!instr.is_nop()) break;  // pure nop ldtmu only
        if (!instr.is_ldtmu()) break;
        last = n;
      }

      for (int n = last; n >= first; n--) {
        auto &instr = instructions[n];
        int shift_to = 0;
         
        int max_shift = -1;  // If set > 0, max number of instructions to look forward

        int end_m = (int) instructions.size();
        if (max_shift > 0 && end_m > n + max_shift) {
          end_m = n + max_shift;
        }

        for (int m = n + 1; m < end_m; m++) {
          auto &instr2 = instructions[m];
          if (instr2.skip()) continue;
          if (instr2.is_branch()) break;   // Paranoia safeguard; it might actually be possible
          if (instr2.uses_sig_dst()) break;
          if (have_dependency(instr, instr2)) break;

          // Specials
          if (!instr2.is_branch()) 
            if (instr2.alu.add.op == V3D_QPU_A_BARRIERID    // In program end, syncs last TMU operation

            //
            // Following empirically determined
            //

            // TMU read and load don't mix
            || (instr2.alu.add.waddr == V3D_QPU_WADDR_TMUA && instr2.alu.add.magic_write)

            // Apparently, can't combine setting flags with TMU loads
            || (instr2.flags.ac != V3D_QPU_COND_NONE || instr2.flags.mc != V3D_QPU_COND_NONE)
            ) {
            break;
          }
          
          shift_to = m;
        }

        if (shift_to != 0) {
          auto &instr2 = instructions[shift_to];
          instr2.sig.ldtmu = true;
          instr2.sig_addr  = instr.sig_addr;
          instr2.sig_magic = instr.sig_magic;
          instr.skip(true);
        }
      }

      i += (last - first);
      continue;
    }


    //
    // Skip instructions that have both add and mul alu
    //
    if (!instr1.add_nop() && !instr1.mul_nop()) continue;
    if (!instr2.add_nop() && !instr2.mul_nop()) continue;


		// auf/mf not used yet, but warn me if it happens
		if (instr1.flag_uf_set() || instr2.flag_uf_set()) {
			warn << "Flag uf set! "
    			 << "line " << i << ":\n"
        	 << "  " << instr1.mnemonic(false) << "\n"
           << "  " << instr2.mnemonic(false)
			;
		}

		// This is absolutely needed to make For/If in Tri work
		// Not sure if second commented out part is absolutely necessary, till now it works
    if (instr1.flag_cond_set()) { // || instr2.flag_cond_set()) { 
			continue;
		}

    if (instr1.flag_push_set() || instr2.flag_push_set()) { 
			// Can't push flags on both add and mul - in principle this is logical
			// Apparently, this case occurs
			// The only way I can see this happen is if a instr is shifted up more than 1 slot.
			continue;
		}

		//
		// TODO: both the following can be done better by splitting add/mul flags
		//

	  // Avoid setting for both cond and push flag
    if (instr1.flag_push_set() && instr2.flag_push_set()) continue;

		// Don't combine cond and push
		// Note that order is important - The other way should be OK (assuming that a push precedes)
    if (instr1.flag_push_set() && instr2.flag_cond_set()) continue;

    if (instr1.flag_push_set() && instr2.flag_push_set()) continue;
    if (instr1.flag_cond_set() && instr2.flag_cond_set()) continue;


    //
    // Combine add and mul instructions, if possible
    //
    bool do_converse;
    if (!can_combine(instr1, instr2, do_converse)) continue;
/*
		Log::warn  << "combine() considering "
    					 << "line " << i << ":\n"
        			 << "  " << instr1.mnemonic(false) << "\n"
               << "  " << instr2.mnemonic(false)
		;
*/		


    // attempt the conversion
    {
      auto const &add_instr = do_converse?instr2:instr1;
      auto const &mul_instr = do_converse?instr1:instr2;

      assert(add_instr.mul_nop());

      v3d::instr::Instr dst = add_instr;

      bool success = Combine::add_alu_to_mul_alu(mul_instr, dst);

      if (success) {
				Log::debug << "Converted: " << dst.mnemonic(false);

        instr1.skip(true);
        instr2 = dst;

        combine_count++;
        i++;
      } else {
				Log::debug << "combine of following failed; "
		    					 << "line " << i << ":\n"
		        			 << "  " << instr1.mnemonic(false) << "\n"
		               << "  " << instr2.mnemonic(false)
				;
			}
      continue;
    }

    break; // Deal with unhandled case in this loop as we encounter them
 	}

  compile_data.num_instructions_combined += combine_count;

  if (combine_count > 0) {
    cdebug << "Combined " << combine_count << " v3d instructions";
  }
}


#ifdef QPU_MODE

void load_uniforms(Data &unif, int numQPUs, Data const &devnull, Data const &done, IntList const &params) {
  int offset = 0;

  // Add the common uniforms
  unif[offset++] = 0;                     // qpu number (id for current qpu) - 0 is for 1 QPU
  unif[offset++] = numQPUs;               // num qpu's running for this job
  unif[offset++] = devnull.getAddress();  // Memory location for values to be discarded

  for (int j = 0; j < params.size(); j++) {
    unif[offset++] = params[j];
  }

  // The last item is for the 'done' location;
  unif[offset] = (uint32_t) done.getAddress();
}

#endif  // QPU_MODE


void invoke(int numQPUs, Data &devnull, Code &codeMem, IntList &params) {
#ifndef QPU_MODE
  assertq(false, "Cannot run v3d invoke(), QPU_MODE not enabled");
#else
  assert(!codeMem.empty());

  Data unif(params.size() + 4);
  Data done(1);
  done[0] = 0;

  load_uniforms(unif, numQPUs, devnull, done, params);

  Driver drv;
  drv.add_bo(getBufferObject().getHandle());
  drv.execute(codeMem, &unif, numQPUs);
#endif  // QPU_MODE
}

}  // anon namespace


///////////////////////////////////////////////////////////////////////////////
// Class KernelDriver
///////////////////////////////////////////////////////////////////////////////

KernelDriver::KernelDriver() : V3DLib::KernelDriver(V3dBuffer) { //, qpuCodeMem(code_bo)  { // Why is last item  here?
	assert(!Platform::compiling_for_vc4());

	if(Platform::compiling_for_vc7()) {
		cdebug << "selecting vc7 as kernel type";
		m_type = vc7;
	} else {
		cdebug << "selecting vc6 as kernel type";
		m_type = vc6;
	}
}


void KernelDriver::encode() {
  if (instructions.size() > 0) return;  // Don't bother if already encoded
  if (has_errors()) return;              // Don't do this if compile errors occured
  assert(!qpuCodeMem.allocated());

  // Encode target instructions
  _encode(m_targetCode, instructions);

  Combine::remove_useless(instructions);

	// When disabled, RNN runs on vc7
	//warn << instructions.dump();
  Combine::combine(instructions);
  Combine::remove_skips(instructions);

	if (!Platform::compiling_for_vc7()) {  // Doesn't work (any more) on vc7
	  combine(instructions);
	  Combine::remove_skips(instructions);
	}

  removeLabels(instructions);

  if (!instructions.check_consistent()) {
    std::string err;
    err << "Overlapping dst registers present";
    local_errors << err;
  }

  if (!local_errors.empty()) {
    breakpoint
  }

  errors << local_errors;
  local_errors.clear();
}


/**
 * Generate the opcodes for the currrent v3d instruction sequence
 *
 * The translation/removal of labels happens here somewhere 
 */
ByteCode KernelDriver::to_opcodes() {
  assert(instructions.size() > 0);
  return instructions.bytecode();
}


void KernelDriver::compile_intern() {
  obtain_ast();
  translate_stmt(m_targetCode, m_body);
  assert(!m_targetCode.empty());

  insertInitBlock(m_targetCode);
  add_init(m_targetCode);

/*
  for (int i = 0; i < m_targetCode.size(); i++) {
		auto const &instr = m_targetCode[i];
		if (instr.tag == LI && instr.LI.imm == 40) {
      warn << "compile_intern pre: " << instr.mnemonic();
    }
  }
*/

	adjust_immediates(m_targetCode);

  // Perform register allocation
  v3d_SourceTranslate().regAlloc(m_targetCode);

  // Satisfy target code constraints
  v3d_satisfy(m_targetCode);

  encode();
}


void KernelDriver::allocate() {
  assert(!instructions.empty());

  // Assumption: code in a kernel, once allocated, doesn't change
  if (qpuCodeMem.allocated()) {
    if (instructions.size() > qpuCodeMem.size()) {
			cerr << "KernelDriver::allocate(): Discrepancy between instruction size: " << (int) instructions.size()
  	       << " and qpuCodeMem size: " << qpuCodeMem.size();
		}
    assert(instructions.size() <= qpuCodeMem.size());  // Tentative check, not perfect
                                                       // actual opcode seq can be smaller due to removal labels
  } else {
    std::vector<uint64_t> code = to_opcodes();
    assert(!code.empty());

    // Allocate memory for the QPU code
    //uint32_t size_in_bytes = (uint32_t) (sizeof(uint64_t)*code.size());
    //code_bo.alloc(size_in_bytes);
    qpuCodeMem.alloc((uint32_t) code.size());
    qpuCodeMem.copyFrom(code);  // Copy kernel to code memory
  }
}


void KernelDriver::invoke_intern(int numQPUs, IntList &params) {
	if (numQPUs <= 0) {
	    cerr << "Zero or negative QPU's selected" << thrw;
	}

	if (Platform::compiling_for_vc7()) {
	  if (numQPUs > 16) {
	    cerr << "Num QPU's exceeded; Max QPU's is 16 for vc7" << thrw;
		}
	} else {
	  if (numQPUs != 1 && numQPUs != 8) {
	    cerr << "Num QPU's must be 1 or 8 for vc6" << thrw;
	  }
	}

  assertq(!has_errors(), "v3d kernels has errors, can not invoke");

  allocate();
  assert(qpuCodeMem.allocated());

  if (!devnull.allocated()) {
    devnull.alloc(16);
  }

  v3d::invoke(numQPUs, devnull, qpuCodeMem, params);
}


void KernelDriver::emit_opcodes(FILE *f) {
  fprintf(f, "Opcodes for %s\n", kernel_type_str().c_str() );
  fprintf(f, "===============\n\n");

  if (instructions.empty()) {
    fprintf(f, "<No opcodes to print>\n");
  } else {
    // You could also print from the code SharedArray,
    // but then you lose the comments.
    for (auto const &instr : instructions) {
      fprintf(f, "%s\n", instr.mnemonic(true).c_str());
    }
  }

  fprintf(f, "\n");
  fflush(f);
}

}  // namespace v3d
}  // namespace V3DLib
