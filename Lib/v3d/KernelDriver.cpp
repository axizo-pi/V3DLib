#include "KernelDriver.h"
#include <iostream>
#include <memory>
#include "Source/Translate.h"
#include "Target/instr/Mnemonics.h"
#include "Target/SmallLiteral.h"  // decodeSmallLit()
#include "Target/RemoveLabels.h"
#include "instr/Snippets.h"
#include "Support/basics.h"
#include "Support/Helpers.h"      // contains()
#include "Support/Timer.h"
#include "SourceTranslate.h"
#include "instr/Encode.h"
#include "instr/Mnemonics.h"
#include "instr/OpItems.h"
#include "instr/BaseSource.h"
#include "global/log.h"
#include "Target/Satisfy.h"
#include "Combine.h"
#include "LibSettings.h"

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

  if (src_instr.ALU.op.value() != Enum::A_BOR && src_instr.ALU.op.value() != Enum::A_MOV) {  // A_MOV is vc7-specific
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

  if (src_instr.ALU.op.value() != Enum::A_BOR) {
    return false;
  }

  auto srca = src_instr.ALU.srcA;
  auto srcb = src_instr.ALU.srcB;
  bool a_is_special = (srca.is_reg() && srca.reg().tag == SPECIAL && srca.reg().regId == index);
  bool b_is_special = (srcb.is_reg() && srcb.reg().tag == SPECIAL && srcb.reg().regId == index);

  return (a_is_special && b_is_special);
}


bool handle_special_index(V3DLib::Instr const &src_instr, Instructions &ret) {
  if (src_instr.tag == ALU && src_instr.ALU.op == Enum::A_TMUWT) {
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


namespace {

Instructions tmua_brainfart(Mnemonic &instr, bool prev_is_tmud) {
  Instructions ret;

  bool changed = false;

  // prev_is_tmud: Do for write only
  //warn << "is tmua: " << (instr.add_dest() == tmua);

  if (Platform::compiling_for_vc7() && instr.add_dest() == tmua) {
    if (!prev_is_tmud) {
      //warn << "Doing tmua brainfart";

      // Following thrsw and nop's absolutely required on vc7, verified
      instr.thrsw();
      ret << instr << nop() << nop();
      changed = true;
    }
  }

  if (!changed) {
    ret << instr;
  }

  return ret;
}

} // anon namespace


bool translateOpcode(Target::Instr const &src, Instructions &ret) {
  static bool prev_is_tmud = false;

  if (handle_special_index(src, ret)) {
    prev_is_tmud = false;
    return true;
  }

  bool did_something = true;

  auto reg_a  = src.ALU.srcA;
  auto reg_b  = src.ALU.srcB;
  auto op     = src.ALU.op.value(); 
  int num_ops = src.ALU.num_operands();

  assertq(op != Enum::A_FSIN || (reg_a.is_reg() && reg_b.is_reg()), "sin has smallims");

  auto dst_reg = encodeDestReg(src);
  assert(dst_reg);


  switch (op) {
    case Enum::A_FSIN:   assert(num_ops == 1); ret << fsin(*dst_reg, reg_a);   break;
    case Enum::A_FFLOOR: assert(num_ops == 1); ret << ffloor(*dst_reg, reg_a); break;
    case Enum::A_TMUWT:  assert(num_ops == 0); ret << tmuwt();                 break;
    case Enum::A_EIDX:   assert(num_ops == 0); ret << eidx(*dst_reg);          break;

    case Enum::A_TIDX:
      breakpoint  // Apparently never called?
      assert(num_ops == 0);
      ret << tidx(*dst_reg);
    break;

    case Enum::A_MOV: {
      assert(num_ops == 1);

      auto tmp = mov(*dst_reg, reg_a);
      ret << tmua_brainfart(tmp, prev_is_tmud);
    }
    break;

    default: {
      // Handle general case
      Mnemonic tmp;

      if (!tmp.alu_set(src)) {
        did_something = false;
        break;
      }

      bool changed = false;

      if (Platform::compiling_for_vc7() && *dst_reg == tmua) {
        assert(!reg_a.is_imm());
        //warn << "default src: " << src.dump();
        //warn << "default tmp: " << tmp.dump();

        if (op == Enum::A_ADD && reg_b.is_imm()) {
          //warn << "translateOpcode(): add(tmua, dst, imm) does not work for vc7, adjusting";

          // It is very probable that the PointExpr addition has been done beforehand,
          // but is not being used. Ignoring that for now. TODO: examine 

          // Do an interim add so that a mov can be used for tmua
          std::unique_ptr<Location> src_p = encodeSrcReg(reg_a.reg());
          auto tmp2  = add(*src_p, reg_a, reg_b);
          auto tmp3  = mov(*dst_reg, reg_a);
          auto tmp4  = sub(*src_p, reg_a, reg_b);

          Instructions tmp_ret;
          tmp_ret << tmp2
                  << tmua_brainfart(tmp3, prev_is_tmud)
                  << tmp4;

          ret << tmp_ret;

          changed = true;
        } else {
          //warn << "default brainfart";
          ret << tmua_brainfart(tmp, prev_is_tmud);
          changed = true;
        }
      }

      if (!changed) {
        ret << tmp;
      }

      break;
    }
  }

  if (did_something) {
    prev_is_tmud = (*dst_reg == tmud);
    //warn << "did_something: " << prev_is_tmud;
    return true;
  } else {
    warn << "NOT did_something";
  }

  auto const &src_alu = src.ALU;
  cerr  << "op: " << src_alu.op.value()
        << ", instr: " << src.dump()
        << thrw;

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

  auto str = src_instr.comment();
  bool is_final_where_cond = contains(str, "where condition final");

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

  Instr tmp_instr;
  auto reg = encodeDestReg(src_instr);
  assert(reg);
  tmp_instr = nop().sub(*reg, *reg, SmallImm(0)).pushz();

  ret << tmp_instr;

/*
  warn << "handle_condition_tags() "
       << "v3d final: " << ret.back().mnemonic() << "'\n"
       << "v3d tmp: " << tmp_instr.mnemonic() << "'\n";
*/       
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
    //cdebug << "translateRotate vc7 input instr: " << instr.dump();

    assert(dst_reg->is_rf());
    assert(src_a->is_rf());
    assert(reg_b.is_imm());  // rf not handled yet
    assert(0 <= reg_b.imm().intVal() && reg_b.imm().intVal() < 16);;

    // vc7 rotates in the other direction! Hoodathunk.
    // Reverse the rotation order
    int tmp_b = 16 - reg_b.imm().intVal();
  
    Instr dst_instr;
    dst_instr.alu.add.op = V3D_QPU_A_ROT;
    dst_instr.alu_add_set(*dst_reg, *src_a, tmp_b);

    //warn << "translateRotate vc7 instruction: " << dst_instr.mnemonic(false);

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
  //Log::debug << "Called v3d encodeInstr()";
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

    //
    // Regular instructions 
    //
    case LI:                ret << encode_LI(instr);   break;
    case ALU:               ret << encodeALUOp(instr); break;
    case InstrTag::BARRIER: ret << barrier();          break;
    case NO_OP:             ret << nop();              break;

    default:
      fatal("v3d: missing case in encodeInstr");
  }

  assert(!ret.empty());

  if (instr.has_comments()) {
    if (ret.empty()) {
      Log::warn << "encodeInstr() comments not transferred, no output instructions";
    } else {
      //Log::warn << "v3d encodeInstr() transferring comments: " << instr.mnemonic(true);
      ret.front().transfer_comments(instr);
    }
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
 *
 * @param instrs  Input list of Target instructions
 * @param dst     output list of v3d instructions
 */
void _encode(V3DLib::Instr::List const &instrs, Instructions &dst) {
  warn << "Called _encode()";
#ifdef DEBUG  
  assertq(checkUniformAtTop(instrs), "_encode(): checkUniformAtTop() failed (v3d)", true);
#endif

  // Main loop
  for (int i = 0; i < instrs.size(); i++) {
    V3DLib::Instr instr = instrs[i];
    assertq(!instr.isZero(), "Zero instruction encountered", true);
    check_instruction_tag_for_platform(instr.tag, false);

    if (instr.tag == INIT_BEGIN) {
    } else if (instr.tag == INIT_END) {
      dst << encode_init();
    } else {
      Instructions ret = v3d::encodeInstr(instr);
      dst << ret;
    }
  }

  dst << sync_tmu()
      << end_program();
}


#ifdef QPU_MODE

void load_uniforms(Data &unif, int numQPUs, Data const &devnull, Data const &done, IntList const &params) {
  int offset = 0;

  // Add the common uniforms
  unif[offset++] = 0;                     // qpu number (id for current qpu) - 0 is for 1 QPU
  unif[offset++] = numQPUs;               // num qpu's running for this job
  unif[offset++] = devnull.getAddress();  // Memory location for values to be discarded

  for (int j = 0; j < params.size(); j++) {
    //Log::warn << "load_uniforms param " << j << ": " << params[j];
    unif[offset++] = params[j];
  }

  // The last item is for the 'done' location;
  unif[offset] = (uint32_t) done.getAddress();
}

#endif  // QPU_MODE



}  // anon namespace


///////////////////////////////////////////////////////////////////////////////
// Class KernelDriver
///////////////////////////////////////////////////////////////////////////////

KernelDriver::KernelDriver() : V3DLib::KernelDriver(V3dBuffer) { //, m_code(code_bo)  { // Why is last item  here?
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
  warn << "Called KernelDriver::encode()";

  if (instructions.size() > 0) return;  // Don't bother if already encoded
  if (has_errors()) return;              // Don't do this if compile errors occured
  assert(!m_code.allocated());

  // Encode target instructions
  _encode(m_targetCode, instructions);

#ifdef DEBUG
  // Check if src's are set for the instructions we expect
  for (int i = 0; i < (int) instructions.size(); ++i) {
    auto const &instr = instructions[i];

    if (instr.is_branch()) continue;
    if (instr.add_nop() && instr.mul_nop()) continue;

    if (!instr.add_nop()) {
      int num_oper = Oper::num_operands(instr.alu.add.op);
      if (num_oper > 0) {
        assert(instr.alu_add_a_set());
      }

      if (num_oper > 1) {
        assert(instr.alu_add_b_set());
      }
    }

    if (!instr.mul_nop()) {  // Assumption: always 2 operands
      assert(instr.alu_mul_a_set());
      assert(instr.alu_mul_b_set());
    }

  }
#endif // DEBUG

  compile_data.num_instructions_combined += Combine::optimize(instructions);

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

  assert(m_targetCode.empty());
  encode_target(m_targetCode, m_body);
  assert(!m_targetCode.empty());

  insertInitBlock(m_targetCode);
  add_init_block(m_targetCode);

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
  if (m_code.allocated()) {
    if (instructions.size() > m_code.size()) {
      cerr << "KernelDriver::allocate(): Discrepancy between instruction size: " << (int) instructions.size()
           << " and m_code size: " << m_code.size();
    }
    assert(instructions.size() <= m_code.size());  // Tentative check, not perfect
                                                        // actual opcode seq can be smaller due to removal labels
  } else {
    std::vector<uint64_t> code = to_opcodes();
    assert(!code.empty());

    // Allocate memory for the QPU code
    m_code.alloc((uint32_t) code.size());
    m_code.copyFrom(code);  // Copy kernel to code memory
  }
}


/**
 * Invoke kernel on QPUs
 */
void KernelDriver::invoke(int numQPUs, IntList &params, bool wait_complete) {
#ifndef QPU_MODE
  assertq(false, "Cannot run v3d invoke(), QPU_MODE not enabled");
#else
  assert(params.size() != 0);

  if (has_errors()) {
    fatal("Errors during kernel compilation/encoding, can't continue.");
  }


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
  assert(m_code.allocated());
  assert(!m_code.empty());

  if (!devnull.allocated()) {
    devnull.alloc(16);
  }

  uniforms.alloc(params.size() + 4);
  done.alloc(1);
  done[0] = 0;

  load_uniforms(uniforms, numQPUs, devnull, done, params);

  drv.add_bo(getBufferObject().getHandle());
  drv.execute(m_code, &uniforms, numQPUs, wait_complete);

  //Log::warn << "KernelDriver::invoke() done: " << done.dump(); 
#endif  // QPU_MODE
}


std::string KernelDriver::emit_opcodes() {
  if (instructions.empty()) return "<No opcodes to print>\n";

  bool do_line_numbers = LibSettings::dump_line_numbers();
  std::string ret;

  int count = 0;
  for (auto const &instr : instructions) {
    auto buf = instr.mnemonic(false);
    int size = (int) buf.size();

    ret << instr.emit_header();

    if (do_line_numbers) {
      ret << count << ": ";
    }

    ret << buf 
        << instr.emit_comment(size)
        << "\n";
    count++;
  }

  return ret;
}


void KernelDriver::wait_complete() {
  if (drv.num_handles() == 0) {
    warn << "wait_complete(): nothing to wait for";
    return;
  }

  warn << "wait_complete done: " << done[0];

  warn << "wait_complete(): waiting for completion.";
  drv.wait_bo();
}

}  // namespace v3d
}  // namespace V3DLib
