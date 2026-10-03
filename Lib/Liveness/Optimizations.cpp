#include "Optimizations.h"
#include "Liveness.h"
#include "Support/Platform.h"
#include "Target/Subst.h"
#include "Support/Timer.h"
#include "Support/basics.h"
#include "Support/Helpers.h"  // contains()
#include <iostream>

namespace V3DLib {

using namespace Target;

namespace {

void replace_acc(Instr::List &instrs, RegUsageItem &item, int var_id, int acc_id) {
  Reg current(REG_A, var_id);
  Reg replace_with(ACC, acc_id);
  Range use_range = item.usage();

  for (int i = use_range.first(); i <= use_range.last(); i++) {
    auto &instr = instrs[i];
    if (!instr.has_registers()) continue;  // Doesn't help much

    instr.rename_dest(current, replace_with);
    renameUses(instr, current, replace_with);
  }

  item.reg = replace_with;    
  //warn << "replace_acc replaced item: " << item.dump();
}


/**
 *
 */
int peephole_0(int range_size, Instr::List &instrs, RegUsage &allocated_vars) {
  if (range_size == 0) return 0;  // Does nothing, not bothering

  int subst_count = 0;

  for (int var_id = 0; var_id < (int) allocated_vars.size(); var_id++) {
    auto &item = allocated_vars.get(var_id);

    if (item.reg.tag != NONE) continue;
    if (item.unused()) continue;
    if (item.use_range() != range_size) continue;
    assert(range_size != 0 || item.only_assigned());

    // Guard for this special case for the time being.
    // It should actually be possible to load a uniform in an accumulator,
    // not bothering right now (TODO test).
    if (instrs[item.first_dst()].isUniformLoad()) {
      continue;
    }

    // Check instructions for unused accumulator
    int acc_id = get_free_acc(instrs, item.usage());
    if (acc_id == -1) {
      //info << "peephole_0 range " << range_size << ": No accumulators available";
      continue;
    }

    // Check if the given ACC has not been assigned in the meantime
    if (allocated_vars.check_overlap_usage(Reg(ACC, acc_id), item)) {
      info << "peephole_0 acc_id: " << acc_id << " already in use, can't assign";
      continue;
    }

    // This also writes the used accumulator to the RegUsage list.
    replace_acc(instrs, item, var_id, acc_id);

    subst_count++;
  }


  return subst_count;
}


/**
 * This is a simple peephole optimisation, captured by the following
 * rewrite rule:
 *
 *     i:  x <- f(...)
 *     j:  g(..., x, ...)
 * 
 * ===> if x not live-out of j
 * 
 *     i:  acc <- f(...)
 *     j:  g(..., acc, ...)
 *
 * @param allocated_vars  write param, to register which vars have an accumulator registered
 *
 * @return Number of substitutions performed;
 */
int peephole_1(Liveness &live, Instr::List &instrs, RegUsage &allocated_vars) {
  RegIdSet liveOut;
  int subst_count = 0;

  for (int i = 1; i < instrs.size(); i++) {
    Instr prev  = instrs[i-1];
    if (!prev.has_registers()) continue;  // Doesn't help much

    Instr instr = instrs[i];

    Reg dst = prev.dst_a_reg();
    if (dst.tag == NONE) continue;
    RegId def = dst.regId;

    // Guard for this special case for the time being.
    // It should actually be possible to load a uniform in an accumulator,
    // not bothering right now (TODO test).
    if (instr.isUniformLoad()) {
      continue;
    }

    live.computeLiveOut(i, liveOut);  // Compute vars live-out of instr

    // If 'instr' is not last usage of the found var, skip
    if (!(instr.src_a_regs().member(def) && !liveOut.member(def))) continue;

    // Can't remove this test.
    // Reason: There may be a preceding instruction which sets the var to be replaced.
    //         If 'prev' is conditional, replacing the var with an acc will ignore the previously set value.
    if (!prev.is_always()) continue;

    Reg current(REG_A, def);
    Reg replace_with(ACC, get_free_acc(instrs, Range(i - 1, i)));
    if(replace_with.regId == -1) {
      warn << "peephole_1: No accumulators available";
      continue;
    }

    prev.rename_dest(current, replace_with);
    renameUses(instr, current, replace_with);
    instrs[i-1] = prev;
    instrs[i]   = instr;
    //warn << "peephole_1 post:\n" << "  " << instrs[i-1].dump() << "  " << instrs[i].dump();

    // DANGEROUS! Do not use this value downstream.   
    // Currently stored for debug display purposes only! 
    allocated_vars.get(def).reg = replace_with;    

    subst_count++;
  }

  return subst_count;
}

}  // anon namespace


/**
 * @return true if any replacements were made, false otherwise
 */
bool combineImmediates(Liveness const &live, Instr::List &instrs) {
  bool found_something = false;

  int const LAST_USE_LIMIT = 50;

  // Detect all LI instructions
  for (int i = 0; i < (int) instrs.size(); i++) {
    Instr &instr = instrs[i];
    if (instr.tag != InstrTag::LI) continue;

    if (instr.LI.imm.is_small_imm()) {
      if (instr.dest().is_special()) {
        info << "combineImmediates special dest register, not combinining, "
             << " instr: " << instr.mnemonic(false);
        continue;
      }

      auto const &reg_usage = live.reg_usage().get(instr.dest().regId);
      Range use_range = reg_usage.usage();

      if (reg_usage.assigned_once()) {
        assert(use_range.first() == reg_usage.first_dst());
        bool can_remove = true;

        for (int j = use_range.first() + 1; j <= use_range.last(); j++) {
          auto &instr2 = instrs[j];
          if (instr2.tag != InstrTag::ALU) continue;
          if (!instr2.is_src_reg(instr.dest())) continue;

          // Can't substitute if a differing immediate is already present
          if (instr2.ALU.srcA.is_imm() && instr2.ALU.srcA != instr.LI.imm) {
            can_remove = false;
            continue;
          }

          if (instr2.ALU.srcB.is_imm() && instr2.ALU.srcB != instr.LI.imm) {
            can_remove = false;
            continue;
          }

          // Perform the subst
          if (instr2.ALU.srcA == instr.dest()) {
            instr2.ALU.srcA = instr.LI.imm;
          }

          if (instr2.ALU.srcB == instr.dest()) {
            instr2.ALU.srcB = instr.LI.imm;
          }
        }

        if (can_remove) {
          // Enable this log when working on this function
          //info << "combineImmediates can_remove, "
          //     << "instr: " << instr.mnemonic(false);
          instrs.set_skip(i);
        }
      }

      continue;
    }

    // Scan forward to find replaceable LI's (i.e. LI's with same value in same or child block)
    int last_use = i;

    // Detect subsequent LI instructions loading the same value
    for (int j = i + 1; j < (int) instrs.size(); j++) {
      Instr &instr2 = instrs[j];

      // Don't go over branches, this affects liveness in a bad way
      if (instr2.is_branch()) break;

       // This is here for performance reasons, to avoid fully scanning huge kernels.
      if (last_use + LAST_USE_LIMIT < j) break;

      if (instr2.is_dst_reg(instr.dest())) break;
      if (instr2.tag != InstrTag::LI) continue;
      if (instr2.LI.imm != instr.LI.imm) continue;
      if (!live.cfg().is_parent_block(j, live.cfg().block_at(i))) continue;

      //
      // Find and replace all occurences of the second LI with the first LI instruction
      //
      int num_subsitutions = 0;
      Reg current      = instr2.dest();
      Reg replace_with = instr.dest();

      // Limit search range to reg usage, or until end of block
      int last = live.cfg().block_end(j);
      {
        RegUsage const &reg_usage = live.reg_usage();
        assert(!reg_usage.get(current.regId).unused());
        int last_usage = reg_usage.get(current.regId).usage().last();
       
        if (last > last_usage) last = last_usage;
      }

      for (int k = j + 1; k <= last; k++) {
        Instr &instr3 = instrs[k];
        if (!instr3.has_registers()) continue;

        if (instr3.is_dst_reg(current)) {
          break;  // Stop if var to replace is rewritten
        }
/*
        Log::debug << "Renaming instr:\n"
                   << "current     : " << i << ": " << current.dump()         << "\n"
                   << "instr3      : " << k << ": " << instr3.mnemonic(false) << "\n"
                   << "replace_with:   "    << ": " << replace_with.dump()    << "\n";
*/
        if (renameUses(instr3, current, replace_with)) {
          num_subsitutions++;
        }
      }

      if (num_subsitutions > 0) {
        last_use = j;
        //Log::debug << "Setting skip on instruction at " << j;
        instrs.set_skip(j);
      }
    }

    found_something = true;
  }

  return found_something;
}


/**
 * @brief Optimization passes that introduce accumulators.
 *
 * This is not called for `vc7`, which has no accumulators.
 *
 * @param allocated_vars write param; note which vars have an accumulator registered
 * @return               Number of substitutions performed
 *
 * ============================================================================
 * NOTES
 * =====
 *
 * 1. It is possible that a variable gets used multiple times, and the last usage of it
 *    is replaced by an accumulator.
 *
 *    For this reason, it is dangerous to keep track of the substitutions in `allocated_vars`,
 *    and to ignore the variable replacement due to acc usage later on. There may still be instances
 *    of the variable that need replacing.
 *
 * 2. MAX_RANGE_SIZE:
 *
 *    - == 0: does nothing, should be >= 2 for any effective use
 *    - >  4: vc4 Unit tests fail, various locations. No free accumulators.
 *    - >= 8: vc4 `insertMoves()` fails, no acc's.
 *    - >= 9: vc4 `peephole_1()` does nothing. Call still works <=12 for vc6.
 *    - >= 10
 *      * tmp var in sin_v3d() gets replaced
 *      * still picks up something >= 10, but not much
 *    - > 12: vc6 barely any hits, not bothering 
 */
int introduceAccum(Liveness &live, Instr::List &instrs) {
	assert(!Platform::compiling_for_vc7());
  timers.start("introduceAccum");
  RegUsage &allocated_vars = live.reg_usage();

  // Num iterations peephole_0. See Note 2.
  int const MAX_RANGE_SIZE = Platform::compiling_for_vc4()?
     4: // vc4
    12; // vc6

#ifdef DEBUG
  //
  // Paranoia safeguards
  //
  for (int i = 0; i < (int) allocated_vars.size(); i++) {
    auto &item = allocated_vars.get(i);

    //reg's should not be allocated already
    assert(item.reg.tag == NONE);
/*
    // TODO fix this on vc6
    // Single range has only a dst register set
    if (item.use_range() == 1) {
      assert(item.assigned_once());
    }
*/
    //
    // Warn me when a variable is dst-only and has multiple dst's.
    // See class RegUsageItem Note 1.
    //
    // vc6: QPU Id and QPU Num will not be flagged as a special case, where possible.
    //
    const int QPU_MAX = 15;  // Top of QPU Id/Num test. Value empirically determined

    if (item.only_assigned() && item.use_dst().size() > 1) {
      // vc6: QPU Id and QPU Num special case
      if (Platform::compiling_for_vc6() && item.use_dst()[0] == 0 && item.use_dst().back() <= QPU_MAX) {
        continue;
      }

      {
        bool found_something = false;
        std::string buf;
        buf << "Multiple dst's: " << i << ": " << item.dump() << "\n";

        // Show the lines where this happens
        for (int dst: item.use_dst()) {
			    // RECV _does_ occur and is benign. Warn me of other cases.
          if (instrs[dst].tag != RECV) {
            buf << "  Line " << dst << ": " << instrs[dst].mnemonic(false) << "\n";
            found_something = true;
          }
        }

        if (found_something) warn << buf;
      }
    }
  }
#endif // DEBUG

  std::string subst_buf;
  int subst_count = 0;

  //
  // Picks up a lot usually
  //
  for (int range_size = 1; range_size <= MAX_RANGE_SIZE; range_size++) {
    int count = peephole_0(range_size, instrs, allocated_vars);

    subst_buf << "  " << range_size << ": " << count << "\n";
    subst_count += count;
  }
  subst_buf << "\n";

  // 
  // This peephole still does useful stuff.
  //
  // Tons of substitutions when peephole_0 disabled.
  // Reversing peephole_0 and _1 introduces more issues than it resolves.
  //
  // Does something, depending on value `MAX_RANGE_SIZE` (vc4 and vc6).
  // 
  {
    int count = peephole_1(live, instrs, allocated_vars);
    //if (count > 0) warn << "peephole_1 did something! count: " << count;
    subst_buf << "peephole_1: " << count << "\n";
    subst_count += count;
  }


  info << "\n===========================================\n"
          "introduceAccum substitution counts\n"
          "----------------------------------\n"
       << "peephole_0 max: " << MAX_RANGE_SIZE << "\n"
       << subst_buf
       << "===========================================\n";

  timers.stop("introduceAccum");
  return subst_count;
}

}  // namespace V3DLib
