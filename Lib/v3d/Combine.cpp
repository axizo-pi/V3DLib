#include "Combine.h"
#include "Support/Platform.h"
#include "Support/basics.h"
#include "instr/BaseSource.h"
#include "instr/Mnemonics.h"  // instr::tmua
#include <set>

/** 
 * @file
 * Routines for optimizing v3d instructions.
 *
 * The main operations here:
 *
 * - Remove useless instructions
 * - Combine alu op's so that both `add` and `mul` are use in a single operation.
 *
 * Unfortunately, not all works well. Most of the time, routines are disabled
 * to keep the library stable.
 */

using namespace Log;

namespace V3DLib {
namespace v3d {

/**
 * @namespace Combine
 *
 * Interface to `v3d` instruction optimization.
 */
namespace Combine {

using V3DLib::v3d::instr::BaseSource;
using ::operator<<;  // C++ weirdness

namespace {

/**
 * vc6-specific.
 *
 * @return true if there are no conflicts with small imm, false otherwise.
 */
bool check_small_imm_usage(Instr const &top, Instr const &bottom) {
  if (Platform::compiling_for_vc7()) {
    // Small imm's currently only handled for vc6

    // If I enable this, combine() catches the exception
    //  and the kernel actually runs on vc7
    //assertq(false, "nope");

    // Hypothesis: only 1 src field can be small imm.
    // This comes from the way the sig fields are set, which are not disjunct.
    // Note that a/b's are not checked against each other
    //
    // The hypothesis appears to be correct.
    return !(top.has_small_imm() && bottom.has_small_imm());

  }

  // vc6

  int top_add_a_imm = -1;
  int top_add_b_imm = -1;

  int bottom_add_a_imm = -1;
  int bottom_add_b_imm = -1;

  if (top.alu_add_a().is_small_imm()) top_add_a_imm = top.alu_add_a().val();
  if (top.alu_add_b().is_small_imm()) top_add_b_imm = top.alu_add_b().val();

  if (bottom.alu_add_a().is_small_imm()) bottom_add_a_imm = bottom.alu_add_a().val();
  if (bottom.alu_add_b().is_small_imm()) bottom_add_b_imm = bottom.alu_add_b().val();

  if (top_add_a_imm != -1 && top_add_b_imm != -1) {
    assertq(top_add_a_imm == top_add_b_imm,
      "register_conflicts: top add a and b must have same small imm");
  }

  if (bottom_add_a_imm != -1 && bottom_add_b_imm != -1) {
    assertq(bottom_add_a_imm == bottom_add_b_imm,
      "register_conflicts: top add a and b must have same small imm");
  }

  if (top_add_a_imm != -1 && bottom_add_a_imm != -1) {
    if (top_add_a_imm != bottom_add_a_imm) return false; 
  }

  if (top_add_a_imm != -1 && bottom_add_b_imm != -1) {
    if (top_add_a_imm != bottom_add_b_imm) return false; 
  }

  if (top_add_b_imm != -1 && bottom_add_a_imm != -1) {
    if (top_add_b_imm != bottom_add_a_imm) return false; 
  }

  if (top_add_b_imm != -1 && bottom_add_b_imm != -1) {
    if (top_add_b_imm != bottom_add_b_imm) return false; 
  }

  return true;
}


/**
 * @brief Check if there is an issue with reading and writing registers
 *
 * @return true if there is a conflict, false otherwise
 *
 * --------------------------------------------------------------
 *
 * - If the registers are disjunct, there is no problem
 * - reads from same register are never a problem
 * - a read before a write can be combined, but the write can not
 *   precede the read
 * - a write before a read can not be equalled or surpassed
 * - 2 writes must be sequential
 *
 * - sig writes (ldtmu) must also be taken into account
 * - immediates must also be notes, especially for vc6 where
 *   only imm_b can be used.
 */
bool add_register_conflict(Instr const &top, Instr const &bottom, bool check_surpass) {
  assert(!bottom.add_nop());

  if (!top.add_nop()) {
    if (top.alu_add_dst() == bottom.alu_add_dst()) return true;

    if (top.alu_add_dst() == bottom.alu_add_a()
     || top.alu_add_dst() == bottom.alu_add_b()) return true;

    if (bottom.alu_add_dst() == top.alu_add_a() || bottom.alu_add_dst() == top.alu_add_b()) {
      if (check_surpass) return true;
    }
  }

  if (!top.mul_nop()) {
    if (top.alu_mul_dst() == bottom.alu_add_dst()) return true;

    if (top.alu_mul_dst() == bottom.alu_add_a()
     || top.alu_mul_dst() == bottom.alu_add_b()) return true;

    if (bottom.alu_add_dst() == top.alu_mul_a() || bottom.alu_add_dst() == top.alu_mul_b()) {
      if (check_surpass) return true;
    }
  }

  if (bottom.alu_add_a() == top.sig_dst()
   || bottom.alu_add_b() == top.sig_dst()
   || bottom.alu_add_dst() == top.sig_dst()
  ) {
    return true;
  }


  if (check_surpass) return false;

  if (!check_small_imm_usage(top, bottom)) return true;

/*
  This was a nice idea but doesn't work well.
  It works better to attempt the conversion and just let it fail.
*/
  // Specific for vc6: have only 2 raddr values for the whole instr
  if (!Platform::compiling_for_vc7()) {
    int count = 0;
    std::set<BaseSource> rf_set;

    if (top.alu_add_a().uses_global_raddr()) { rf_set.insert(top.alu_add_a()); count++; }
    if (top.alu_add_b().uses_global_raddr()) { rf_set.insert(top.alu_add_b()); count++; } 
    if (top.alu_mul_a().uses_global_raddr()) { rf_set.insert(top.alu_mul_a()); count++; }
    if (top.alu_mul_b().uses_global_raddr()) { rf_set.insert(top.alu_mul_b()); count++; } 
    if (bottom.alu_add_a().uses_global_raddr()) { rf_set.insert(bottom.alu_add_a()); count++; } 
    if (bottom.alu_add_b().uses_global_raddr()) { rf_set.insert(bottom.alu_add_b()); count++; } 

    if (count > 2 && ((int) rf_set.size() <= 2)) {
      std::string buf;
      for (auto &n: rf_set) {
        buf << n.dump() << ", ";
      }
    }

    if (rf_set.size() > 2) {
      return true;
    }
  }

  return false;
}


/**
 * @brief Convert add op to mul op if possible.
 *
 * @param mul_op    output param; mul op if conversion succesfull.
 * @param add_instr input param; instruction with add op to convert
 *
 * @return true if conversion possible, false otherwise
 *         If possible, output param mul_op is set.
 *
 * --------------------------------------------------
 * 
 * Possible conversions add <-> mul (from mesa2 qpu_instr.h):
 *
 *     V3D_QPU_A_ADD  <-> V3D_QPU_M_ADD,
 *     V3D_QPU_A_SUB  <-> V3D_QPU_M_SUB,
 *     V3D_QPU_A_FMOV <-> V3D_QPU_M_FMOV,  // vc7 rhs only
 *     V3D_QPU_A_MOV  <-> V3D_QPU_M_MOV,   // vc7 rhs only
 *     V3D_QPU_A_NOP  <-> V3D_QPU_M_NOP,
 */
bool add_op_to_mul_op(v3d_qpu_mul_op &mul_op, v3d::instr::Instr const &add_instr) {
  // Op's are common to both vc6 and vc7 unless otherwise specified
  switch (add_instr.alu.add.op) {
    case V3D_QPU_A_OR: {
      if (Platform::compiling_for_vc7()) {
        assert(add_instr.alu.add.a.mux == add_instr.alu.add.b.mux);
      } else {
        assert(add_instr.alu.add.a.raddr == add_instr.alu.add.b.raddr);
      }

      mul_op = V3D_QPU_M_MOV;
      return true;
    }

    case V3D_QPU_A_ADD:  mul_op = V3D_QPU_M_ADD;  return true;
    case V3D_QPU_A_SUB:  mul_op = V3D_QPU_M_SUB;  return true;
    case V3D_QPU_A_FMOV: mul_op = V3D_QPU_M_FMOV; return true;
    case V3D_QPU_A_MOV:  mul_op = V3D_QPU_M_MOV;  return true;  // vc7 only
    case V3D_QPU_A_NOP:  mul_op = V3D_QPU_M_NOP;  return true; // Included for completeness

    default: break;
  }

  return false;
}


/**
 * @brief Check if an add operation can be a mul-operation.
 *
 * This does not check if the current instruction has
 * an empty mul-op. The move could be to any instruction,
 * including self.
 *
 * @return true if add op in `instr` could be mul. False otherwise.
 */
bool alu_add_could_be_mul(Instr const &instr) {
  v3d_qpu_mul_op mul_op;
  return add_op_to_mul_op(mul_op, instr);
}


MAYBE_UNUSED bool can_equal(Instr const &top, Instr const &bottom) {
  if (add_register_conflict(top, bottom, false)) return false;

  v3d_qpu_mul_op tmp;
  if (!add_op_to_mul_op(tmp, bottom)) return false ;

  return top.mul_nop();
}


MAYBE_UNUSED bool can_surpass(Instr const &top, Instr const &bottom) {
  return !add_register_conflict(top, bottom, true);
}


/**
 * @internal
 * This can't fail as long as both instructions are alu add
 */
void alu_add_copy_src(Instr const &src, Instr &dst) {
  assert(!src.add_nop());
  assert(!dst.add_nop());
  assert(src.mul_nop());  // Paranoia
  assert(dst.mul_nop());

  dst.alu_add_a(src.alu_add_a(), true);
  dst.alu_add_b(src.alu_add_b(), true);
}


/**
 * @brief Combine two instructions to an add/mul instruction.
 *
 * The in_instr must either be an add or mul instruction, not both.
 * The dst must be an add instruction only.
 *
 * There are two options:
 *   1. the add instruction in in_instr can be translated to a mul instruction
 *   2. in_instr contains only a mul instruction which can be moved to dst
 *
 * There are pleny of limitations which are taken into account here.
 *
 * @param src  instruction that will be combined
 * @param dst  instruct that will contain the add and mul instruction (inshalla)
 * @return     true if combine succeeded, false otherwise
 */
bool alu_to_mul_alu(Instr const &src, Instr &dst) {
  assert((!src.add_nop() &&  src.mul_nop()) || (src.add_nop() && !src.mul_nop())); 
  if (!dst.mul_nop()) return false; 

  Instr ret = dst;

  //
  // Get used dst and src
  //
  BaseSource alu_add_a = src.alu_add_a();
  BaseSource alu_add_b = src.alu_add_b();
  BaseSource alu_mul_a = src.alu_mul_a();
  BaseSource alu_mul_b = src.alu_mul_b();

  if (src.mul_nop()) {  // Take the values from alu add
    v3d_qpu_mul_op mul_op;
    if (!add_op_to_mul_op(mul_op, src)) return false;

    ret.alu.mul.op = mul_op;
    auto dst_loc = src.add_alu_dst();
    ret.alu_mul_dst(*dst_loc);  // dst (waddr) is always safe

    if (!ret.alu_mul_a(alu_add_a)) return false;
    if (!ret.alu_mul_b(alu_add_b)) return false;

  } else {                  // Take the values from alu mul
    ret.alu.mul.op = src.alu.mul.op;
    auto dst_loc = src.mul_alu_dst();
    ret.alu_mul_dst(*dst_loc);

    if (!ret.alu_mul_a(alu_mul_a)) {
      return false;
    }

    if (!ret.alu_mul_b(alu_mul_b, true)) {
      return false;
    }
    ret.alu_mul_b(alu_mul_b, true);
  }

  if (src.mul_nop()) {
    ret.alu.mul.output_pack = src.alu.add.output_pack;
    ret.alu.mul.a.unpack    = src.alu.add.a.unpack;
    ret.alu.mul.b.unpack    = src.alu.add.b.unpack;

    ret.flags.mc  = src.flags.ac;
    ret.flags.mpf = src.flags.apf;
    ret.flags.muf = src.flags.auf;
  } else {
    ret.alu.mul.output_pack = src.alu.mul.output_pack;
    ret.alu.mul.a.unpack    = src.alu.mul.a.unpack;
    ret.alu.mul.b.unpack    = src.alu.mul.b.unpack;

    ret.flags.mc  = src.flags.mc;
    ret.flags.mpf = src.flags.mpf;
    ret.flags.muf = src.flags.muf;
  }

  ret.header(src.header());
  ret.comment(src.comment());

  dst = ret;
  return true;
}


/**
 *
 * @param instr1 First instruction to combine
 * @param instr2 Second instruction to combine
 * @param ret    Output parameter; contains result if combine succeeds
 *
 * @return true instructions are combined, false otherwise.
 *         If true, param ret is set.
 *
 * ---------------------------------------------
 * Current implementation deal with cases where one instruction is add and the other mul.
 *
 * Possible combinations:
 *
 * - Combines only possible if instr1 and instr2 have at most two operations together.
 * - 'nt' postfix: operation is not transferrable to other side (add -> mul or vice versa).
 *   Left out if not relevant.
 *
 * I really made an exhaustive list and this is what remains!
 *
 *      instr1         instr2
 *      -----------   -----------
 *      nop  ,nop     nop  ,nop    - trivial cases
 *      nop  ,nop     nop  ,mul
 *      nop  ,nop     add  ,nop
 *      nop  ,nop     add  ,mul
 *      nop  ,mul     nop  ,nop
 *      add  ,nop     nop  ,nop
 *      add  ,mul     nop  ,nop
 *
 *      nop  ,mul     add  ,nop    - Handled here
 *      add  ,nop     nop  ,mul
 *
 *      nop  ,mul     nop  ,mul
 *      nop  ,mul     nop  ,mulnt
 *      nop  ,mulnt   nop  ,mul
 *      nop  ,mulnt   nop  ,mulnt  - Can't combine
 *
 *      add  ,nop     add  ,nop
 *      add  ,nop     addnt,nop
 *      addnt,nop     add  ,nop
 *      addnt,nop     addnt,nop    - Can't combine
 * ---------------------------------------------
 *
 * - **TODO**: 'sig_addr' usage should also be added to the list!
 *   This field is independent of src/dst. The only issue is when it interferes with the dst fields.
 * - Non-transferrable add-ops have been filtered out beforehand.
 * - In theory, you could check on transferrable mul-ops.
 */
bool combine_instruction(Instr &ret, Instr const &instr1, Instr const &instr2) {
  // Can't combine if too many operations present
  int op_count = 0;
  if (!instr1.add_nop()) ++op_count;
  if (!instr1.mul_nop()) ++op_count;
  if (!instr2.add_nop()) ++op_count;
  if (!instr2.mul_nop()) ++op_count;
  if (op_count >= 3) return false;

  assert(!instr1.is_branch());  // So it's an alu instr
  assert(!instr2.is_branch());  // idem

  if (instr1.skip() || instr2.skip()) {
      warn << "combine_instruction assert on skip fails:\n"
           << "  instr2: " << instr2.mnemonic()    << "\n"
           << "  instr1: " << instr1.mnemonic();
  }
  assert(!instr1.skip());
  assert(!instr2.skip());

  // a single instruction with flags should be OK. No flags even better
  if (instr1.has_flags() && instr2.has_flags()) return false; 

  assert(!instr1.is_label());   // This could actually be handled by transferring the label
  assert(!instr2.is_label());   // idem. Warn me when these happen

  //
  // Deal with signals when they happen.
  //
  // Signal: It should be possible to let a **single** tmu load happen.
  //
  // - Small imm flags **must be handled**.
  // - Also, rotate sig may need separate handling. Not sure if used at all
  //
  assert(!instr1.sig.rotate);
  assert(!instr2.sig.rotate);

  if (instr1.has_signal() || instr2.has_signal()) {
      warn << "combine_instruction assert on signals fails:\n"
           << "  instr2: " << instr2.mnemonic()    << "\n"
           << "  instr1: " << instr1.mnemonic();
  }
  assert(!instr1.has_signal());
  assert(!instr2.has_signal());

  // m_external_init skipped, should be OK 

  // Input should each have one alu instruction
  // For the time being, one is add and the other is mul
  bool switch_instr = false;

  if (instr1.mul_nop()) {
    // instr1 must be add op
    assert(!instr1.add_nop());

    if (!instr2.add_nop()) {
      // both instr1 and instr2 have add op's
      if (!alu_add_could_be_mul(instr1) && !alu_add_could_be_mul(instr2)) return false;

      warn << "combine_instruction assert fails:\n"
           << "  instr2: " << instr2.mnemonic()    << "\n"
           << "  instr1: " << instr1.mnemonic();
    }
    assert(instr2.add_nop());

    // instr1 has add, instr2 nop add
    // add can be moved up, irrespective of mul op, which could be nop
    switch_instr = false;
  } else {
    // instr1 must be mul op
    assert(instr1.add_nop());
    assert(!instr2.add_nop());
    assert(instr2.mul_nop());
    switch_instr = true;
  }
  
  // Set output to add op
  Instr mul = switch_instr?instr1:instr2;
  ret       = switch_instr?instr2:instr1;

  assert(!mul.alu_add_a_set());
  assert(!mul.alu_add_b_set());
  assert(!ret.alu_mul_a_set());
  assert(!ret.alu_mul_b_set());

  //
  // The actual combine
  //

  //sig_addr and sig_magic intentionally skipped, will be picked up when sig assert above fires.

  if (!Platform::compiling_for_vc7()) {
    // vc6: Needs special tests for raddr fields
    // raddr_a/b can be ignored for vc7
    assert(false);  // Deal with this when it happens
  }

  // Handle small imm
  if (Platform::compiling_for_vc7()) {
    // vc7
    if (ret.has_small_imm()) {
      if (mul.has_small_imm()) return false;   // multiple small imm's can *never* be combined
    } else if (mul.has_small_imm()) {
      assert(!ret.has_small_imm());
      ret.sig.small_imm_c = mul.sig.small_imm_c;
      ret.sig.small_imm_d = mul.sig.small_imm_d;
    }
  } else {
    // vc6
    assert(false);  // Deal with this when it happens
  }

  // Copy mul flags 
  ret.flags.mc  =  mul.flags.mc;
  ret.flags.mpf =  mul.flags.mpf;
  ret.flags.muf =  mul.flags.muf;

  ret.alu.mul = mul.alu.mul;
  // branch is in a union with alu, don't copy
  // Comments are handled  downstream

  return true;
}


/**
 * @internal
 *
 * @return true if instruction passes filter
 */
bool filter_move(Instr const &instr) {
  if (instr.skip()) return false;

  if (
    instr.is_branch() ||
    instr.add_nop() ||
    !instr.mul_nop()       // Note that mul is skipped here
  ) {
    return false;
  }

  if (instr.flag_push_set()) return false;
  if (instr.flag_cond_set()) return false;

  return true;
}


bool is_move(Instr const &instr) {
  auto op = instr.alu.add.op;

  if (op == V3D_QPU_A_OR) {
    auto a = instr.alu_add_a();
    assert(a.is_set());
    auto b = instr.alu_add_b();
    assert(b.is_set());

    return (a == b);
  }

  // vc7
  if (op == V3D_QPU_A_MOV) {
    auto a = instr.alu_add_a();
    assert(a.is_set());  // Paranoia checking
    return true;
  }

  return false;
}


int remove_skips(Instructions &instr) {
  Instructions ret;
  int skip_count = 0;

  for (int i = 0; i < (int) instr.size(); i++) {
    auto &cur = instr[i];

    if (cur.skip()) {
      auto &next = instr[i + 1];
      next.transfer_comments(cur);

      skip_count++;
    } else {
      ret << cur;
    }
  }

  if (skip_count > 0) {
    instr = ret;
  }

  return skip_count;
}


/**
 * Remove instructions which do not do anything.
 *
 * Example:
 *
 *     mov rf0, rf0
 *
 * @param instr  List of instruction to check for being useless
 * @return       Number of useless instructions to skip
 */
int remove_useless(Instructions &instr) {
  //
  // Detect useless moves, eg: or  rf2, rf2, rf2    ; nop
  //
  auto check_useless_moves = [] (Instr const &instr, int line_number) -> bool {
    if (!filter_move(instr)) return false;

    auto op = instr.alu.add.op;

    // The only op's handled here
    if (
      op != V3D_QPU_A_OR  &&
      op != V3D_QPU_A_MOV &&
      op != V3D_QPU_A_ADD
    ) {
      return false;
    }

    auto dst = instr.alu_add_dst();
    assert(dst.is_set());
    auto a = instr.alu_add_a();
    assert(a.is_set());

    if (op == V3D_QPU_A_OR) {
      if (instr.has_signal(true)) return false; 

      // a and b both used
      auto b = instr.alu_add_b();
      assert(b.is_set());

      if (!(a == b && dst == a)) return false; 
    }
    
    if (op == V3D_QPU_A_MOV) {
      // Only add a is used
      if (instr.has_signal(true)) return false; 
      if (!(a == dst)) return false; 
    }


    // eg add(dst, dst, 0), add(dst, 0, 0)
    if (op == V3D_QPU_A_ADD) {
      if (instr.has_signal(false)) return false; 

      bool ret = false;

      auto b = instr.alu_add_b();
      assert(b.is_set());

      if (a == dst) {
        if (b.is_small_imm() && b.val() == 0) ret = true; 
      }

      if (b == dst) {
        if (a.is_small_imm() && a.val() == 0) ret = true; 
      }

      if (!ret) return false;
    }

    return true;
  };

  assertq(!check_useless_moves(instr[0], 0), "First instruction is useless");

  for (int i = 0; i < (int) instr.size(); i++) {
    auto &cur = instr[i];
    if (cur.skip()) continue;

    //
    // Remove useless copies
    // For some reason, the first instr (index 1) does not get picked up
    //
    if (check_useless_moves(cur, i)) {
      //warn << "Skipping useless move: " << cur.mnemonic(false);
      cur.skip(true);
      continue;
    }
  }

  //  
  // Detect combines moves to dst. eg:
  //    or(tmp, src, src)
  //    or(dst, tmp, tmp)
  //
  // Doing related moves works better if useless moves done first
  //
  // Does something useful on vc6.
  //
  if (false) // <==============
  for (int i = 0; i < (int) instr.size(); i++) {
    auto &cur = instr[i];
    if (cur.skip()) continue;

    if (i < (int) instr.size() - 1) {
      auto &nxt = instr[i + 1];
      if (nxt.skip()) continue;
      if (!filter_move(cur)) continue;
      if (!filter_move(nxt)) continue;

      if (is_move(cur) && is_move(nxt)) {
        if (cur.has_signal(false)) continue; 
        if (nxt.has_signal(false)) continue;

        if  ((cur.alu_add_dst() != nxt.alu_add_a())) continue;

        auto cur_str = cur.mnemonic();
        auto nxt_str = nxt.mnemonic();

        // Scan forward to see if cur dst is used somewhere
        bool dst_is_src = false;
        instr::DestReg dst =  cur.add_dest();

        for (int j = i + 2; j < (int) instr.size(); ++j) {  // Skip nxt, this one is known
          auto tmp = instr[j];

          if (dst == tmp.add_dest() || dst == tmp.sig_dest()) {
            //warn << "Found cur dst '" << dst.dump() << "' as dst at " << j << "; " << tmp.mnemonic();
            // This is OK
            break;
          }

          if (dst == tmp.add_src_a() || dst == tmp.add_src_b()) {
            warn << "Found cur dst '" << dst.dump() << "' as src at " << j << "; " << tmp.mnemonic();
            dst_is_src = true;
            break;
          }
        }

        if (dst_is_src) {
          //warn << "Cur dst in use as src, can not combine";
          continue;
        }


        alu_add_copy_src(cur, nxt);
        cur.skip(true);

        if (check_useless_moves(nxt, i + 1)) {
          warn << "Result is useless!:\n"
               << "  nxt: " << nxt.mnemonic();

          nxt.skip(true);
          continue;
        }

        warn << "Combined move OR at " << i << "\n"
             << "  cur     : " << cur_str   << "\n"
             << "  nxt     : " << nxt_str   << "\n"
             << "  combined: " << nxt.mnemonic();
      }
    }
  }

  return remove_skips(instr);
}


namespace {

///////////////////////////////////////////////////////
// Support for combine()
//
// Goals:
//
// - Raise level of abstraction
// - Isolate functionality for other applications.
//   E.g. I want to combine add-ops on the app-ALU
//   (instead of moving add-opst to mul-ALU).
///////////////////////////////////////////////////////

/**
 * @brief Find the start of the main program.
 *
 * This skips the loading of the uniforma and the general initialization.
 * If start not found, combine will run from the start of the program.
 *
 * @return index of instruction where actual program starts,
 *         0 otherwise.
 *
 * -------------------
 *
 * Notes
 * -----
 *
 * This was a while-loop and sometimes resulted in error:
 * 
 *      Cannot create a lazy string with address 0x0, and a non-zero length.
 *
 * The hypothesis is that the search went over the end of the instructions list.
 *
 */
int find_program_start(Instructions const &instr) {
  int start = -1;

  for (int i = 0; i < (int) instr.size(); ++i) {
    if (instr[i].header() == "Main program") {
      start = i;
      break;
    }
  }

  if (start == -1) {
    warn << "Could not find header 'Main program'";
    start = 0;
  }

  return start;
}


/**
 * Check if given instruction should be skipped in the combine evaluation.
 *
 * Will also throw for certain combinations.
 *
 * @return true if instruction should be skipped, false otherwise
 */
bool skip_instruction(Instr const &bottom, int line_index) {
  if (bottom.add_nop() ) return true; 
  if (!bottom.mul_nop()) return true; 
  if (bottom.skip()    ) return true; 

  if (bottom.add_nop() && bottom.mul_nop()) {
    // Skip full NOP's
    return true;
  }

  if (!bottom.add_nop() && !bottom.mul_nop()) {
    // instruction efficient already, don't bother (both add and mul used).
    // This could in fact be optimized by moving up add and/or mul
    // separately; not going there right now.
    return true;
  }

  if (
    bottom.flag_push_set() ||
    bottom.flag_cond_set() ||
    bottom.flag_uf_set() 
  ) {
    // Deal with flags later on
    return true;;
  }

  if (bottom.alu_add_dst() == instr::tmua) {
    return true;
  }

  if (bottom.has_magic_registers()) {
    cdebug << "magic write to special register set on instr:\n"
           << "  " << bottom.mnemonic() << "\n"
           << "  - Deal with this later on";
    return true;
  }

  if (bottom.has_signal()) {
    if (!bottom.sig.thrsw && !bottom.uses_sig_dst()) {
      // Warn me about any other signals
      warn << "Deal with signals later on, instr:\n"
           << line_index << ": " << bottom.mnemonic() << "\n" << thrw;
      return true;
    }
  }

  return false;
}  


/**
 * @brief Check if dst of lhs is same as any src of rhs.
 *
 * All combinations of add and mul are checked here.
 *
 * @return true if lhs dst used as rhs src,
 *         false otherwise.
 */
bool dst_matches_src(Instr const &lhs, Instr const &rhs) {
  return (
    (lhs.alu_add_dst() == rhs.alu_add_a() || lhs.alu_add_dst() == rhs.alu_add_b()) ||
    (lhs.alu_add_dst() == rhs.alu_mul_a() || lhs.alu_add_dst() == rhs.alu_mul_b()) ||
    (lhs.alu_mul_dst() == rhs.alu_add_a() || lhs.alu_mul_dst() == rhs.alu_add_b()) ||
    (lhs.alu_mul_dst() == rhs.alu_mul_a() || lhs.alu_mul_dst() == rhs.alu_mul_b()) ||
    (lhs.sig_dst()     == rhs.alu_add_a() || lhs.alu_mul_dst() == rhs.alu_add_b()) ||
    (lhs.sig_dst()     == rhs.alu_mul_a() || lhs.alu_mul_dst() == rhs.alu_mul_b())
  );
}


/**
 * @internal
 * All dst's are handled here.
 */
bool dst_fields_overlap(Instr const &top, Instr const &bottom) {
  return (
    (top.alu_add_dst() == bottom.alu_add_dst())  ||
    (top.alu_add_dst() == bottom.alu_mul_dst())  ||
    (top.alu_add_dst() == bottom.sig_dst()    )  ||
    (top.alu_mul_dst() == bottom.alu_add_dst())  ||
    (top.alu_mul_dst() == bottom.alu_mul_dst())  ||
    (top.alu_mul_dst() == bottom.sig_dst()    )  ||
    (top.sig_dst()     == bottom.alu_add_dst())  ||
    (top.sig_dst()     == bottom.alu_mul_dst())  ||
    (top.sig_dst()     == bottom.sig_dst()    )
  );
}


/**
 * @brief Check if bottom add op can equal or surpass the top add/mul op.
 *
 * **NOTE**: Resolve overlap with `add_register_conflict()`.
 *
 * @return true if can equal or surpass, false otherwise
 */
bool add_can_equal_or_surpass(Instr const &top, Instr const &bottom) {
  assert(!bottom.add_nop());  // expand to bottom mul later on
  assert(bottom.mul_nop());   // idem

  //
  // Write operations should never ever be moved
  //
  using namespace V3DLib::v3d::instr;

  if (!bottom.add_nop()) {
    // Following for vc7
     if (bottom.alu.add.op == V3D_QPU_A_MOV) { 
      if ((bottom.alu_add_dst() == tmua) || (bottom.alu_add_dst() == tmud)) return false;
    }

    // Following more likely for vc6; also for completeness on vc7
     if (bottom.alu.mul.op == V3D_QPU_M_MOV) { 
      if ((bottom.alu_mul_dst() == tmua) || (bottom.alu_mul_dst() == tmud)) return false;
    }

     if (bottom.alu.add.op == V3D_QPU_A_TMUWT) return false;
  }

  //
  // Dst registers can not be the same.
  // Also can not assign bottom before top.
  //
  if (dst_fields_overlap(top, bottom)) {
    //cdebug << "add_can_equal_or_surpass() dst bottom and top same, can't equal or surpass.";
    return false;
  }


  //
  // Check src and dst
  //

  // if bottom src same as top dst, can't equal or surpass,
   if (dst_matches_src(top, bottom)) {
    return false;
  }

  // if bottom dst same as top scr, can't surpass.
  // It *could* equal. TODO look at this later
  if (dst_matches_src(bottom, top)) {
    cdebug << "add_can_equal_or_surpass() bottom dst is top src, can't equal or surpass.";
    return false;
  }

  return true;
}


/**
 * @brief Check if the bottom add op can be combined in the top instruction.
 *
 * @return true if can equal , false otherwise
 */
bool add_can_equal(Instr const &top, Instr const &bottom) {

  //
  // Check small immediates - applies to both add and mul
  //
  if (bottom.has_small_imm() && top.has_small_imm() ) {
    // Can be surpassed for both vc6 and vc7.

    if (Platform::compiling_for_vc7()) {
      // Never equal; small imm's can never be combined for vc7.
      return false;
    } else {
      // vc6: if small imm values are the same, it's OK.
     if (bottom.small_imm_value() != top.small_imm_value()) {
       return false;
     }
    }
  }


  //
  // Check combining operations
  //
  assert(!bottom.add_nop());  // Warn me if this happens

  if (!top.add_nop()) {
    // Can one add op be moved out of the way?
    if (alu_add_could_be_mul(top) && top.mul_nop()) {
      return true;
    }

    if (alu_add_could_be_mul(bottom) && top.mul_nop()) {  // Note: *top* mul checked for NOP
      cdebug << "add_can_equal() bottom add could be moved to top mul.";
      return true;
    }

    return false;
  }

  return true;
}


/**
 * @brief Skip a branch instruction.
 *
 * If the passed instruction `bottom` is a branch, skip to launch point.  
 * Nothing happens if this is not a branch.
 *
 * @param bottom     instruction to check for branch.
 * @param line_index current instruction indec. Will be adjusted
 *                   to the launch point if a branch detected.
 * @return           true if branch detected, false if not present.
 */
bool skip_to_launch_point(Instr const &bottom, int &line_index) {
  if (bottom.is_branch()) {
    // For some reason, an extra NOP is added after a branch
    int const Skip = 5;

    line_index += Skip + 1;
    return true;
  }

  return false;
}


/**
 * Move bottom add op to top mul op.
 *
 * This is initally to combine mov tmud/tmua for tmu write,
 * but I'm trying hard to make it as general as possible.
 *
 * @param bottom First instruction to combine
 * @param top Second instruction to combine
 * @param ret    Output parameter; contains result if combine succeeds
 *
 * @return true instructions are combined, false otherwise.
 *         If true, param ret is set.
 */
bool bottom_add_to_top_mul(Instr &ret, Instr const &bottom, Instr const &top) {
  // Restrict input ot our use case (tmud/tmua)
  assert(!bottom.add_nop());
  assert(bottom.mul_nop());
  assert(!top.add_nop());
  assert(top.mul_nop());

  // Following are to warn me when it happens
  assert(!bottom.has_small_imm());
  assert(!top.has_small_imm());
  assert(Platform::compiling_for_vc7());  // vc6 must be handled separately

  if (!(top.mul_nop() && alu_add_could_be_mul(bottom))) return false;

  //
  // Do actual combine
  //
  ret = top;

  // Copy add flags 
  ret.flags.mc  =  bottom.flags.ac;
  ret.flags.mpf =  bottom.flags.apf;
  ret.flags.muf =  bottom.flags.auf;

  v3d_qpu_mul_op mul_op;
  if (!add_op_to_mul_op(mul_op, bottom)) return false;

  ret.alu.mul.op          = mul_op;
  ret.alu.mul.a           = bottom.alu.add.a;
  ret.alu.mul.b           = bottom.alu.add.b;
  ret.alu.mul.waddr       = bottom.alu.add.waddr;
  ret.alu.mul.magic_write = bottom.alu.add.magic_write;
  ret.alu.mul.output_pack = bottom.alu.add.output_pack;

  return true;
}


/**
 * @brief Optimize read/write operations for `v3d`.
 *
 * @param instr  List of instructions to scan.
 *
 * -------------------
 *
 * **Read operation:**
 *
 *     mov tmua, rf20                ; nop                         ; thrsw
 *     nop                           ; nop                         
 *     nop                           ; nop                         
 *     nop                           ; nop                         ; ldtmu.rf20
 *
 * There must be *at least* 2 `nop`'s between the `mov tmua` and the `ldtmu`.
 * It can be more, e.g. when using `gather/receive`.
 *
 * The issue here is the overhead of the `nop`'s.
 * Things to do with this:
 *
 * - Move up lower instructions to fill up the `nop`'s.
 *   This is something that the regular `combine()` instruction can deal with.
 * - Put `tmua mov`'s after each other.
 *   This should work fine as long as the TMU FIFO is not filled
 *   (in which case, the reads will block).
 *
 * **Write operation:**
 *
 *     mov tmud, rf23                ; nop                         # store_var v3d
 *     mov tmua, rf20                ; nop                         
 *     tmuwt rf63                    ; nop     
 *
 * The goal in this method is to combine the `mov`'s:
 *
 *     mov tmud, rf23                ; mov tmua, rf20              # store_var v3d
 *     tmuwt rf63                    ; nop     
 *
 * Notes
 * =====
 *
 * 1. The combination of the `mov`'s for the read **does not work**  
 *    The instruction merge is accepted by the compile, but _there is no output_.  
 *    Even worse, this persists when the kernel is re-run.
 *
 * This indicates that there is some state issue involved here, on the hardware level
 * (I can not rule out that it is a code issue).  
 * Running other kernels in between, however, works fine. I am perplexed.  
 * _NB:_ Restarting the Pi(5) fixes this. It is perplexing nevertheless.
 *
 * switching `mov tmua/tmud` does not solve anything. It's worse, because, the
 * kernel call does not return.
 *
 * **TODO**: Find a way to reset the `vc7` hardware.
 *           This exists for `vc6` and I have implemented it.
 *           However, this does **not** work for `vc7`.
 *    
 */
MAYBE_UNUSED void combine_read_write(Instructions &instr) {
  assertq(false, "Don't call combine_read_write() for now, too many issues");

  warn << "\n----------------------------\n"
       <<   "Entered combine_read_write()\n"
       <<   "----------------------------";

  using namespace V3DLib::v3d::instr;

  int end = (int) instr.size();

  auto is_tmua = [] (Instr const &instr) -> bool {
     return (instr.alu.add.op == V3D_QPU_A_MOV  && instr.alu_add_dst() == tmua);
  };

  auto is_tmud = [] (Instr const &instr) -> bool {
     return (instr.alu.add.op == V3D_QPU_A_MOV  && instr.alu_add_dst() == tmud);
  };

  auto is_tmwt = [] (Instr const &instr) -> bool {
     return (instr.alu.add.op == V3D_QPU_A_TMUWT);
  };

  for (int i = 0; i < end; ++i) {
    auto &n = instr[i];

    if (is_tmud(instr[i]) && is_tmua(instr[i + 1]) && is_tmwt(instr[i + 2])) {
      warn << "combine_read_write tmu write detected\n"
           << "  " << i       << ": " << instr[i].mnemonic(true)      << "\n"
           << "  " << (i + 1) << ": " << instr[i + 1].mnemonic(true)  << "\n"
           << "  " << (i + 2) << ": " << instr[i + 2].mnemonic(true)
      ;

if (false) {      
      //
      // Move tmua to mul op. Not working, see Note 1!
      //
      auto &bottom = instr[i + 1];
      auto &top    = instr[i];
      Instr ret;

      if (bottom_add_to_top_mul(ret, bottom, top)) {
        warn << "combine success:\n"
             << "  " << ret.mnemonic(true)      << "\n";

        top = ret;
        bottom.skip(true);
      } else {
        assert(false); // Deal with this when it happens
      }
}      

      i += 2; // NB ++ in for
      continue;
    }

    if (is_tmua(n)) {
      // Check integrity tmu read

      if (!n.sig.thrsw) {
        warn << "combine_read_write assert sig failed\n"
             << "  " << i       << ": " << n.mnemonic(true)        << "\n";
      }
      assert(n.sig.thrsw);

      // Following assumes that the ldtmu is placed exactly here; this is
      // not true for gather/receive.
      assert(instr[i + 3].sig.ldtmu);

      warn << "combine_read_write tmu read detected\n"
           << "  " << i << ": " << n.mnemonic(true);
      ;

      i += 3; // NB ++ in for
      continue;
    }

    // Not expecting these separately after previous steps
    assert(!is_tmud(n));
    assert(!is_tmua(n));
    assert(!is_tmwt(n));
  }
}

}  // anon namespace



MAYBE_UNUSED void combine(Instructions &instr) {
  warn << "\n----------------------------\n"
       <<   "Entered combine()\n"
       <<   "----------------------------";

  int start = find_program_start(instr);

  // TODO: better specify end program. This should be on barrierid
  int end = (int) instr.size();

  // Consider all instructions in main body
  cdebug << "combine(): starting loop; start: " << start << ", end: " << end;
  for (int i = start + 1; i < end; ++i) {
    auto &bottom = instr[i];

    if (skip_instruction(bottom, i)) continue;

    if (skip_to_launch_point(bottom, i)) {
      start = i;          // Can't move instructions upward beyond this point
      assert(i <  end);
      continue;
    }

    int final_top = -1; // If set, a candidate is found

    //
    // Try to move instructions as far up as possible
    //
    for (int j = i - 1; j >= start; --j) {
      auto &top    = instr[j];

      if (top.skip()) continue;
      if (bottom.add_nop() && bottom.mul_nop()) break;    // Nothing to do 

      //
      // Check bottom add op
      //
      if (bottom.add_nop()) break; // Don't bother. This must be changed for mul op

      if (!add_can_equal_or_surpass(top, bottom)) {
        break; // No point in continuing
      }

      //
      // top already filled, no point
      //
      // This **must** be after add_can_equal_or_surpass(),
      // because src/dst's still must be tested
      //
      if (!top.add_nop() && !top.mul_nop()) continue;

      if (!add_can_equal(top, bottom)) {
        continue;
      }
      final_top = j;
/*
      cdebug << "Considering following:\n"
             << "  " << j << ": " << top.mnemonic()    << "\n"
             << "  " << i << ": " << bottom.mnemonic()
      ;
*/

    }

    if (final_top == -1) continue;

    //
    // Move forward again to find the best place
    //
    for (int k = final_top; k < i; ++ k) {
      auto &ftop = instr[k];

      if (ftop.skip()) continue;

      std::string buf;
      buf << "  " << k << ": " << ftop.mnemonic()    << "\n"
          << "  " << i << ": " << bottom.mnemonic();

      if (ftop.mul_nop() && alu_add_could_be_mul(bottom)) {
/*        
        // TODO examine this when all is stable
        cdebug << "Move forward could move bottom add to ftop mul. Skipping for now\n"
               << "  " << k << ": " << ftop.mnemonic()    << "\n"
               << "  " << i << ": " << bottom.mnemonic();
*/               
        continue;
      }

      if (ftop.mul_nop() && alu_add_could_be_mul(ftop)) {
/*        
        // TODO examine this when all is stable
        cdebug << "Move forward could move ftop add to ftop mul. Skipping for now\n"
               << "  " << k << ": " << ftop.mnemonic()    << "\n"
               << "  " << i << ": " << bottom.mnemonic();
*/               
        continue;
      }

       if (dst_matches_src(bottom, ftop)) {
        cdebug << "Move forward bottom dst is top src, could combine but skipping for now.\n"
               << "  " << k << ": " << ftop.mnemonic()    << "\n"
               << "  " << i << ": " << bottom.mnemonic();
        continue;
      }

      Instr ret;
      if (combine_instruction(ret, bottom, ftop)) {
        //
        // Comment handling
        //

        // bottom comment most likely belong to a block; move comment to next op
        instr[i + 1].header(bottom.header());
        instr[i + 1].comment(bottom.comment());

        // retain only the comments of ftop
        ret.clear_comments();
        ret.header(ftop.header());
        ret.comment(ftop.comment());

        warn << "Combine success.\n"
             << "Pre:\n"
             << buf << "\n"
             << "Post:\n"
             << "  " << final_top << ": " << ret.mnemonic() << "\n"
             << "== Combine SUCCESS ==";

        // Consolidate the conversion
        ftop = ret;
        bottom.skip(true);
        break;
      } else {
        continue;
      }

      cdebug << "Move forward again:\n"
             << buf;
      cdebug << "skip got till: " << i;
      return;

      if (ftop.skip()) continue;
      if (!ftop.mul_nop()) continue;

      if (alu_to_mul_alu(bottom, ftop)) {
        cdebug << "combine: adding bottom to top succeeded.\n"
               << "Pre:\n"
               << buf << "\n"
               << "Post:\n"
               << "  " << final_top << ": " << ftop.mnemonic() << "\n"
               << "== Combine SUCCESS =="
        ;

        bottom.skip(true);
        break;
      } else {
        warn << "combine: Tried to combine bottom to top:\n" << buf;
      }
    }
  }
}


/**
 * @brief Check if given instructions have a dependency on each other.
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


/**
 * @internal
 *
 * **NOTE**: small imm not checked, perhaps this gets fixed downstream.
 */
template<typename AddAlu>
bool can_be_mul_alu(AddAlu const &add_alu) {
  if (Platform::compiling_for_vc7()) {
    // Block write to special registers for now
    // Actually, this should be possible (on vc7 at least), as long as the special registers aren't the same
    // on add and mul.
		// TODO examine
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


void combine_old(Instructions &instructions) {
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
    if (instr1.flag_cond_set()) { // || instr2.flag_cond_set())  
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

      bool success = alu_to_mul_alu(mul_instr, dst);

      if (success) {
        Log::debug << "Converted: " << dst.mnemonic(false);

        instr1.skip(true);
        instr2 = dst;

        combine_count++;
        i++;
      } else {
        /*
         This test is working nicely for vc6: trying to fill in >= 3 values in raddr_a/b

        Log::warn << "Combine of following failed; "
                   << "line " << i << ":\n"
                   << "  " << instr1.mnemonic(false) << "\n"
                   << "  " << instr2.mnemonic(false)
        ;
        */
      }
      continue;
    }

    break; // Deal with unhandled case in this loop as we encounter them
   }


  if (combine_count > 0) {
    cdebug << "Combined " << combine_count << " v3d instructions";
  }
}

} // anon namespace


/**
 * @brief Entry point for `v3d` instruction optimization.
 */
int optimize(Instructions &instrs) {
  int count = 0;

try {
  count += remove_useless(instrs);

  if (!Platform::compiling_for_vc7()) {  // Doesn't work (any more) on vc7
    combine_old(instrs);
    count += remove_skips(instrs);
  }

} catch(V3DLib::Exception const &e) {
  cerr << "\nV3DLib exception during Combine::combine(). Aborting further combination. err:\n"
       << e.what();
} catch(std::runtime_error &e) {
  cerr << "\nruntime_error exception during Combine::combine(). Aborting further combination. err:\n"
       << e.what();
} catch(...) {
  cerr << "\nUnknown exception during Combine::combine(). Aborting further combination.\n";
}

  return count;
}

}  // namespace Combine
}  // namespace v3d
}  // namespace V3DLib
