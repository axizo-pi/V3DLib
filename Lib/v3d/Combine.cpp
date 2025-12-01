#include "Combine.h"
#include "Support/Platform.h"
#include "Support/basics.h"
#include "instr/BaseSource.h"
#include "instr/Mnemonics.h"  // instr::tmua
#include <set>

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
  if (Platform::compiling_for_vc7()) {
		// Small imm's currently only handled for vc6

		// If I enable this, combine() catches the exception
	  //	and the kernel actually runs on vc7
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
bool add_register_conflict(Instr const &top, Instr const &bottom, bool check_surpass) {
  assert(!bottom.add_nop());

  // warn << "dst top: " << top.alu_add_dst().dump() << ", bottom: " << bottom. alu_add_dst().dump();

  //
  // TODO: take empty reg's into account
  //       eg. in a mov, the b src is not used
  //
  if (!top.add_nop()) {
  	if (top.alu_add_dst() == bottom.alu_add_dst()) return true;

  	if (top.alu_add_dst() == bottom.alu_add_a()
	   || top.alu_add_dst() == bottom.alu_add_b()) return true;

  	if (bottom.alu_add_dst() == top.alu_add_a() || bottom.alu_add_dst() == top.alu_add_b()) {
    	//warn << "register_conflicts: bottom dst same as top src; take this into account";
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
      warn << "rf_set: (" << buf << ")";
    }

    //Log::warn << "rf_set.size: " << (int) rf_set.size();
    if (rf_set.size() > 2) {
      return true;
    }
  }

/*
  } else {
    // Warn for any other sig's
    if (top.has_signal() || bottom.has_signal()) {
      cerr << "register_conflicts: instructions have signals, not handling for now" << thrw;
    }
  }
*/

  return false;
}


/**
 * Find corresponding mul op for a given add op
 *
 * @return true if found, false otherwise
 */
bool convert_alu_op_to_mul_op(v3d_qpu_mul_op &mul_op, v3d::instr::Instr const &add_instr) {
	// Op's common to both vc6 and vc7
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

    case V3D_QPU_A_ADD:
      mul_op = V3D_QPU_M_ADD;
      return true;

    case V3D_QPU_A_SUB:
      mul_op = V3D_QPU_M_SUB;
      return true;

		// vc7 - This broke Tri previously
    case V3D_QPU_A_MOV:
      mul_op = V3D_QPU_M_MOV;
      return true;

    default: break;
  }

  return false;
}


bool can_equal(Instr const &top, Instr const &bottom) {
  if (add_register_conflict(top, bottom, false)) return false;
/*
	warn << "Here1, top,bottom:\n" 
		   << top.mnemonic() << "\n"
		   << bottom.mnemonic();
*/

  v3d_qpu_mul_op tmp;
  if (!convert_alu_op_to_mul_op(tmp, bottom)) return false ;

  return top.mul_nop();
}


bool can_surpass(Instr const &top, Instr const &bottom) {
  return !add_register_conflict(top, bottom, true);
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


namespace {

//
// Return true if instruction passes filter
//
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

} // anon namespace


void remove_useless(Instructions &instr) {
  //
  // Detect useless moves, eg: or  rf2, rf2, rf2    ; nop
  //
  auto check_useless_moves = [] (Instr const &instr, int line_number) -> bool {
    if (!filter_move(instr)) return false;

		// Check assign to self

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

			// add(dst,0,0) is actually useful! Like mov(dst, 0)
/*
			// Following is actually illegal to run on qpu (qpu encode flags this)
			// However, before complete compile it's possible and seen in the wild
      if (a.is_small_imm() && b.is_small_imm()) {
				if (a.val() == 0 && b.val() == 0) return true; 
			}
*/			

			if (!ret) return false;
    }

    Log::debug
			<< "Useless move at line " << line_number << ": " << instr.mnemonic(false)
			<< " dst: " << dst.dump()
		;

		return true;
  };

  assertq(!check_useless_moves(instr[0], 0), "First instruction is useless");

  for (int i = 0; i < (int) instr.size(); i++) {
    auto &cur = instr[i];
    if (cur.skip()) continue;

    //
    // Remove useless copies
		//
		// For some reason, the first instr (index 1) does not get picked up
    //
    if (check_useless_moves(cur, i)) {
			//warn << "Skipping useless move: " << cur.mnemonic(false);
      cur.skip(true);
      continue;
    }

    //  
    // Detect combines moves to dst. eg:
    //    or(tmp, src, src)
    //    or(dst, tmp, tmp)
    //
    if (i < (int) instr.size() - 1) {
      auto nxt = instr[i + 1];
      if (!filter_move(cur)) continue;
      if (!filter_move(nxt)) continue;

      if (is_move(cur) && is_move(nxt)) {
      	if (cur.has_signal(true)) continue; 
      	if (nxt.has_signal(true)) continue;

        if ((cur.alu_add_dst() != nxt.alu_add_a())) continue;

        warn << "remove_useless detected move OR:\n"
             << "  cur: " << cur.mnemonic() << "\n"
             << "  nxt: " << nxt.mnemonic();
      }
    }
  }

  remove_skips(instr);
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
  //if (end > (start + 133)) end = (start + 133);  // WRI DEBUG
  //if (end > (start + 350)) end = (start + 350);  // WRI DEBUG
  //if (end > (start + 250)) end = (start + 250);  // WRI DEBUG

  // Consider all instructions in main body
  for (int i = start + 1; i < end; ++i) {
    if (instr[i].add_nop()) continue; 
    if (!instr[i].mul_nop()) continue; 
    if (instr[i].skip()) continue; 


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


    if (bottom.has_signal()) {
			if (!bottom.sig.thrsw && !bottom.sig.ldtmu) {
				// Warn me about any other signals
				warn << "Deal with signals later on, instr:\n"
  	         <<  i << "; " << bottom.mnemonic() << "\n" << thrw;
			}
		}

    if (
      bottom.flag_push_set() ||
      bottom.flag_cond_set() ||
      bottom.flag_uf_set() 
    ) {
      //cerr << "Deal with flags later on. instr:\n"
      //     <<  bottom.mnemonic() << thrw;
      continue;
    }


    if (bottom.alu_add_dst() == instr::tmua) {
      //warn << "Skipping tmua mov";
      continue;
    }

    bool has_special_regs =
      bottom.sig_dst().is_magic() || 
      bottom.alu_add_dst().is_magic() || 
      bottom.alu_mul_dst().is_magic() 
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
/*
    if (i - final_top > 1) {
      warn << "final top is " << (i - final_top) << " places above i";
    }
*/

    //
    // Move forward again to find the best place
    //
    for (int k = final_top; k < i; ++ k) {
      auto &ftop    = instr[k];
      if (ftop.skip()) continue;
  
      //bool full_op =  !ftop.add_nop() && !ftop.mul_nop();
      //if (full_op) continue;
      if (!ftop.mul_nop()) continue;

      std::string buf;
      buf << "  " << k << ": " << ftop.mnemonic()    << "\n"
          << "  " << i << ": " << bottom.mnemonic();

      if (add_alu_to_mul_alu(bottom, ftop)) {
/*				
        warn << "combine: adding bottom to top succeeded.\n"
             << "Pre:\n"
             << buf << "\n"
             << "Post:\n"
             << "  " << final_top << ": " << ftop.mnemonic() << "\n"
             << "== Combine SUCCESS =="
        ;
*/				

        bottom.skip(true);
        break;
      } else {
      	warn << "combine: Tried to combine bottom to top:\n" << buf;
      }
    }
  }

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
