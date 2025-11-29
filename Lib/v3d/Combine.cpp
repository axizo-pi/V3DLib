#include "Combine.h"
#include "Support/Platform.h"
#include "Support/basics.h"
#include "instr/BaseSource.h"

using namespace Log;

namespace V3DLib {
namespace v3d {
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
  assertq(!Platform::compiling_for_vc7(), "Small imm's currently only handled for vc6");

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
    ///warn << "comparing imm's top.add.a with bottom.add.b: "
    ///     << top_add_a_imm << " != " << bottom_add_b_imm;
    if (top_add_a_imm != bottom_add_b_imm) return false; 
  }

  if (top_add_b_imm != -1 && bottom_add_a_imm != -1) {
    if (top_add_b_imm != bottom_add_a_imm) return false; 
  }

  if (top_add_b_imm != -1 && bottom_add_b_imm != -1) {
    ///warn << "comparing imm's top.add.a with bottom.add.b: "
    ///     << top_add_a_imm << " != " << bottom_add_b_imm;
    if (top_add_b_imm != bottom_add_b_imm) return false; 
  }

  return true;
}

/**
 * Check if there is an issue with reading and writing registers
 *
 * @return true if there is a conflict, false otherwise
 *
 * ==============================================================
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
bool register_conflict(Instr const &top, Instr const &bottom, bool check_surpass) {
  // add alu only for now

  // warn << "dst top: " << top.alu_add_dst().dump() << ", bottom: " << bottom. alu_add_dst().dump();

  if (top.alu_add_dst() == bottom.alu_add_dst()) {
    // warn << "Dst's check out";
    return true;
  }

  if (top.alu_add_dst() == bottom.alu_add_a()
   || top.alu_add_dst() == bottom.alu_add_b()) return true;

/*
  warn << "\n"
    << "  top.alu_add_dst : " << top.alu_add_dst().dump()  << "\n"
    << "  top.alu_add_a   : " << top.alu_add_a().dump()    << "\n"
    << "  top.alu_add_b   : " << top.alu_add_b().dump()    << "\n"
    << "  bottom.alu_add_a: " << bottom.alu_add_a().dump() << "\n"
    << "  bottom.alu_add_b: " << bottom.alu_add_b().dump()
  ;
*/

  if (bottom.alu_add_dst() == top.alu_add_a() || bottom.alu_add_dst() == top.alu_add_b()) {
    //warn << "register_conflicts: bottom dst same as top src; take this into account";
    if (check_surpass) return true;
  }

  // Assumption: all checks below are for small immediates
  if (check_surpass) return false;

  if (top.has_signal() || bottom.has_signal()) {
    cerr << "register_conflicts: instructions have signals, not handling for now" << thrw;
  }

  return !check_small_imm_usage(top, bottom);
}


/**
 * Find corresponding mul op for a given add op
 *
 * @return true if found, false otherwise
 */
bool convert_alu_op_to_mul_op(v3d_qpu_mul_op &mul_op, v3d::instr::Instr const &add_instr) {
	// Op's common to both vc6 and vc7
  switch (add_instr.alu.add.op) {
    case V3D_QPU_A_OR:
			if ( add_instr.alu.add.a.mux == add_instr.alu.add.b.mux) {
        mul_op = V3D_QPU_M_MOV;
        return true;
      }

    case V3D_QPU_A_ADD:
      mul_op = V3D_QPU_M_ADD;
      return true;

    case V3D_QPU_A_SUB:
      mul_op = V3D_QPU_M_SUB;
      return true;

    default: break;
  }

	// This breaks Tri - absolutely confirmed
/*	
	if (Platform::compiling_for_vc7()) {
  	switch (add_instr.alu.add.op) {
	    case V3D_QPU_A_MOV:
				mul_op = V3D_QPU_M_MOV;
	      return true;

    	default: break;
  	}
	} 
*/

  return false;
}


bool can_equal(Instr const &top, Instr const &bottom) {
  if (register_conflict(top, bottom, false)) return false;

  v3d_qpu_mul_op tmp;
  if (!convert_alu_op_to_mul_op(tmp, bottom)) return false ;

  bool full_op =  !top.add_nop() && !top.mul_nop();
  return !full_op;
}


bool can_surpass(Instr const &top, Instr const &bottom) {
  return !register_conflict(top, bottom, true);
}

} // anon namespace


/**
 * Combine two instructions to an add/mul instruction.
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
 * @param in_instr instruction that will be combined
 * @param dst      instruct that will contain the add and mul instruction (inshalla)
 *
 * @return true if combine succeeded, false otherwise
 */
bool add_alu_to_mul_alu(Instr const &src, Instr &dst) {
	// Perhaps TODO: change these assert's int 'if () return false'
  assert((!src.add_nop() &&  src.mul_nop()) || (src.add_nop() && !src.mul_nop())); 

  if (!dst.mul_nop()) return false; 

  //
  // Get used dst and src
  //
	BaseSource alu_add_a = src.alu_add_a();
	BaseSource alu_add_b = src.alu_add_b();
	BaseSource alu_mul_a = src.alu_mul_a();
	BaseSource alu_mul_b = src.alu_mul_b();
/*
	warn
 		<< "\n"
    << "  src alu_add_a: " << alu_add_a.dump() << "\n"
    << "  src alu_add_b: " << alu_add_b.dump() << "\n"
    << "  src alu_mul_a: " << alu_mul_a.dump() << "\n"
    << "  src alu_mul_b: " << alu_mul_b.dump();
*/

  if (src.mul_nop()) {  // Take the values from alu add
    v3d_qpu_mul_op mul_op;
    if (!convert_alu_op_to_mul_op(mul_op, src)) return false;  // False if no applicable translation add -> mul

		if (!dst.alu_mul_b_safe(alu_add_b)) return false;
		if (!dst.alu_mul_a_safe(alu_add_a)) return false;

  	dst.alu.mul.op = mul_op;
		auto dst_loc = src.add_alu_dst();
  	dst.alu_mul_dst(*dst_loc);  // dst (waddr) is always safe
  	dst.alu_mul_a(alu_add_a);
  	dst.alu_mul_b(alu_add_b);
  } else {                  // Take the values from alu mul
		if (!dst.alu_mul_b_safe(alu_mul_b)) return false;
		if (!dst.alu_mul_a_safe(alu_mul_a)) return false;

    dst.alu.mul.op = src.alu.mul.op;
		auto dst_loc = src.mul_alu_dst();
  	dst.alu_mul_dst(*dst_loc);
  	dst.alu_mul_a(alu_mul_a);
  	dst.alu_mul_b(alu_mul_b);
  }

  if (src.mul_nop()) {
    dst.alu.mul.output_pack = src.alu.add.output_pack;
    dst.alu.mul.a.unpack    = src.alu.add.a.unpack;
    dst.alu.mul.b.unpack    = src.alu.add.b.unpack;

    dst.flags.mc  = src.flags.ac;
    dst.flags.mpf = src.flags.apf;
    dst.flags.muf = src.flags.auf;
  } else {
    dst.alu.mul.output_pack = src.alu.mul.output_pack;
    dst.alu.mul.a.unpack    = src.alu.mul.a.unpack;
    dst.alu.mul.b.unpack    = src.alu.mul.b.unpack;

    dst.flags.mc  = src.flags.mc;
    dst.flags.mpf = src.flags.mpf;
    dst.flags.muf = src.flags.muf;
  }

  dst.header(src.header());
  dst.comment(src.comment());

/*
	Log::warn << "  dst : " << dst.mnemonic(false);
	if (!Platform::compiling_for_vc7()) {
		Log::warn << "mul mux b:" << dst.alu.mul.b.mux  << ", raddr_b: " << dst.raddr_b;
	}
*/
  return true;
}


void remove_skips(Instructions &instr) {
  Instructions ret;
  int skip_count = 0;

  for (int i = 0; i < (int) instr.size(); i++) {
    auto const &cur_instr = instr[i];

    if (cur_instr.skip()) {
      skip_count++;
    } else {
      ret << cur_instr;
    }
  }

  if (skip_count > 0) {
    warn << "Skipped " << skip_count << " instructions";
    instr = ret;
  }
}


void combine(Instructions &instr) {
  // warn << "Combine::combine() called";

try {

  // Find the start of the main program
  int k = 0;
  while (instr[k].header() != "Main program") {
    ++k;
  }
  int start = k;

  // TODO: better specify end program. This should be on barrierid
  int end = (int) instr.size();
  if (end > (start + 50)) end = (start + 50);  // WRI DEBUG

  // Consider all instructions in main body
  for (int i = start + 1; i < end; ++i) {
    if (instr[i].add_nop()) continue; 


    if (instr[i].is_branch()) {
      warn << "Branch detected, skipping to launch point";
      i += 4;
      assert(i <  end);
    }

    auto &bottom = instr[i];

    if (!bottom.add_nop() && !bottom.mul_nop()) {
      // instruction efficient already, don't bother (both add and mul used).
      // This could in fact be optimized by moving up add and/or mul
      // separately; not going there right now.
      continue;
    }

    int final_top = -1;

    // Skipping the hard parts for later on
    assertq(!bottom.skip(), "Deal with skips later on");
    assertq(!bottom.has_signal(), "Deal with signals later on");
    assertq(!(
      bottom.flag_push_set() ||
      bottom.flag_cond_set() ||
      bottom.flag_uf_set() 
    ), "Deal with flags later on");

    bool has_special_regs =
      (bottom.has_signal() && bottom.sig_dest().is_magic()) || 
      (!bottom.add_nop() && bottom.alu.add.magic_write && bottom.alu.add.waddr > V3D_QPU_WADDR_R5) ||
      (!bottom.mul_nop() && bottom.alu.mul.magic_write && bottom.alu.mul.waddr > V3D_QPU_WADDR_R5)
    ;

    if (has_special_regs) {
      cerr << "magic write to special register set on instr:\n"
           << "  " << bottom.mnemonic() << "\n"
           << "- Deal with this later on";
      break;
    }

    // Try to move instructions as far up as possible
    for (int j = i - 1; j >= start; --j) {
     auto &top    = instr[j];

     if (top.skip()) continue;
/*
     warn << "Considering following:\n"
          << "  " << j << ": " << top.mnemonic()    << "\n"
          << "  " << i << ": " << bottom.mnemonic()
      ;
*/

      if (!(
        !bottom.add_nop() && bottom.mul_nop()
      )) {
        cerr << "combine: only add op's in bottom handled now.\n" 
             << "  " << j << ": " << top.mnemonic()    << "\n"
             << "  " << i << ": " << bottom.mnemonic()
             << thrw;
      }

      if (can_equal(top, bottom)) {
        //warn << "bottom can equal top";
        final_top = j;
      }

      if (!can_surpass(top, bottom)) {
        break;  // Don't look further
      }
    }

    if (final_top == -1) continue;

    //if (i - final_top > 1) {
    //  warn << "final top is " << (i - final_top) << " places above i";
    //}

    //
    // Move forward again to find the best place
    //
    for (int k = final_top; k < i; ++ k) {
      auto &ftop    = instr[k];
  
      bool full_op =  !ftop.add_nop() && !ftop.mul_nop();
      if (full_op) continue;

      std::string buf;
      buf << "  " << k << ": " << ftop.mnemonic()    << "\n"
          << "  " << i << ": " << bottom.mnemonic();


      if (add_alu_to_mul_alu(bottom, ftop)) {
        warn << "combine: adding bottom to top succeeded.\n"
             << "Pre:\n"
             << buf << "\n"
             << "Post:\n"
             << "  " << final_top << ": " << ftop.mnemonic() << "\n"
             << "== Combine SUCCESS =="
        ;

        bottom.skip(true);
        //i++;
        break;
      } else {
      warn << "combine: Tried to combine bottom to top:\n" << buf;
      }
    }
  }

// TODO: NOT BEING CAUGHT
} catch(V3DLib::Exception const &e) {
  cerr << "Runtime error during Combine::combine(). Aborting further combination. err:\n"
       << e.what();
} catch(...) {
  cerr << "Unknown untime error during Combine::combine(). Aborting further combination.\n";
}

}

}  // namespace Combine
}  // namespace v3d
}  // namespace V3DLib
