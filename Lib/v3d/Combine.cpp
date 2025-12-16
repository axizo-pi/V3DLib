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
      // warn << "rf_set: (" << buf << ")";
    }

    //Log::warn << "rf_set.size: " << (int) rf_set.size();
    if (rf_set.size() > 2) {
      return true;
    }
  }

/*
  else {
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


/**
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
bool alu_to_mul_alu(Instr const &src, Instr &dst) {
	// Perhaps TODO: change these assert's int 'if () return false'
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
    if (!convert_alu_op_to_mul_op(mul_op, src)) return false;  // False if no applicable translation add -> mul

  	ret.alu.mul.op = mul_op;
		auto dst_loc = src.add_alu_dst();
  	ret.alu_mul_dst(*dst_loc);  // dst (waddr) is always safe

  	if (!ret.alu_mul_a(alu_add_a)) {
			return false;
		}

  	if (!ret.alu_mul_b(alu_add_b)) {
			return false;
		}
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
/*
	Log::warn << "  dst : " << dst.mnemonic(false);
	if (!Platform::compiling_for_vc7()) {
		Log::warn << "mul mux b:" << dst.alu.mul.b.mux  << ", raddr_b: " << dst.raddr_b;
	}
*/
  return true;
}


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


int remove_skips(Instructions &instr) {
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
    //warn << "Skipped " << skip_count << " instructions";
    instr = ret;
  }

	return skip_count;
}


int remove_useless(Instructions &instr) {
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

        if	((cur.alu_add_dst() != nxt.alu_add_a())) continue;

        auto cur_str = cur.mnemonic();
        auto nxt_str = nxt.mnemonic();

        //warn << "remove_useless detected move OR:\n"
        //     << "  cur: " << cur_str << "\n"
        //     << "  nxt: " << nxt_str;

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


void combine(Instructions &instr) {
  // warn << "Combine::combine() called";

try {

  //
  // Find the start of the main program.
  // This was a while-loop and sometimes resulted in error:
  //
	// Cannot create a lazy string with address 0x0, and a non-zero length.
  //
  // The hypothesis is that the search went over the end of the instructions list.
  //
  int start = -1;
  for (int i = 0; i < (int) instr.size(); ++i) {
    if (instr[i].header() != "Main program") {
      start = i;
    }
  }

  if (start == -1) {
    warn << "Could not find header 'Main program'";
    start = 0;
  }

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

      if (alu_to_mul_alu(bottom, ftop)) {
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


int optimize(Instructions &instrs) {
	int count = 0;

  count += remove_useless(instrs);

	// When disabled, RNN runs on vc7
	//warn << instrs.dump();
  combine(instrs);
  count += remove_skips(instrs);

	if (!Platform::compiling_for_vc7()) {  // Doesn't work (any more) on vc7
	  combine_old(instrs);
	  count += remove_skips(instrs);
	}

	return count;
}

}  // namespace Combine
}  // namespace v3d
}  // namespace V3DLib
