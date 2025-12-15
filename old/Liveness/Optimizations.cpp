#include "Optimizations.h"
#include <iostream>
#include "Liveness.h"
#include "Support/Platform.h"
#include "Support/Helpers.h"   // to_file()
#include "Target/Subst.h"
#include "Support/Timer.h"
#include "Support/basics.h"
#include "global/log.h"

using namespace Log;

namespace V3DLib {
namespace {

void replace_acc(Instr::List &instrs, RegUsageItem &item, int var_id, int acc_id) {
  Reg current(REG_A, var_id);
  Reg replace_with(ACC, acc_id);

  for (int i = item.first_usage(); i <= item.last_usage(); i++) {
    auto &instr = instrs[i];
    if (!instr.has_registers()) continue;  // Doesn't help much

    instr.rename_dest(current, replace_with);
    renameUses(instr, current, replace_with);
  }

  // DANGEROUS! Do not use this value downstream.   
  // Currently stored for debug display purposes only! 
  item.reg = replace_with;    
}


/**
 * Not as useful as I would have hoped. range_size > 1 in practice happens, but seldom.
 */
int peephole_0(int range_size, Instr::List &instrs, RegUsage &allocated_vars) {
  if (range_size == 0) {
    warning("peephole_0(): range_size == 0 passed in. This does nothing, not bothering");
    return 0;
  }

  int subst_count = 0;

  for (int var_id = 0; var_id < (int) allocated_vars.size(); var_id++) {
    auto &item = allocated_vars[var_id];

    if (item.reg.tag != NONE) continue;
    if (item.unused()) continue;
    if (item.use_range() != range_size) continue;
    assert(range_size != 0 || item.only_assigned());

    // Guard for this special case for the time being.
    // It should actually be possible to load a uniform in an accumulator,
    // not bothering right now.
    if (instrs[item.first_dst()].isUniformLoad()) {
      continue;
    }

    //
    // NOTE: There may be a slight issue here:
    //       in line of first use, src acc's may be used for vars which have
    //       last use in this line. I.e. they would be free for usage in this line.
    //
    // This is a small thing, perhaps for later optimization
    //
    int acc_id = instrs.get_free_acc(item.first_usage(), item.last_usage());

    if (acc_id == -1) {
/*
      warning("No free acc!");

      std::string msg;
      msg << "range_size: " << range_size << ", Var " << var_id << ", "
          << "lines " << item.first_usage()  << "-" << item.last_usage() << "\n"
          << instrs.check_acc_usage(item.first_usage(), item.last_usage()) << "\n";

      debug(msg);
*/
      continue;
    }

    Reg replace_with(ACC, acc_id);

/*
    // This call is an extreme performance hog and has not failed in recent memory
    // Enable if you're totally paranoid
    allocated_vars.check_overlap_usage(replace_with, item);
*/

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

  //to_file("before_peephole1.txt", instrs.dump(true));

  // Something could be won by redoing the loop if code changed.
  // but then liveness would need to be recomputed.

  for (int i = 1; i < instrs.size(); i++) {
    Instr prev  = instrs[i-1];
    if (!prev.has_registers()) continue;  // Doesn't help much

    Instr instr = instrs[i];

    Reg dst = prev.dst_reg();
    if (dst.tag == NONE) continue;

    // if dst already acc, no point
    if (!dst.is_rf_reg()) continue;

    // No point if prev dst not used in next line
    if (dst != instr.src_a_reg() && dst != instr.src_b_reg()) continue;

    RegId def = dst.regId;

    auto const &reg_item = live.reg_usage()[def];
    int first_src = reg_item.first_src();

    if (first_src == -1) {
      // dst never used, no point in continuing
      continue;
    }

    if (reg_item.last_src() > i) {
      // dst used further on, can't replace
      // Note that this is conservatie, there may be assignments in between
      continue;
    }

    // Guard for this special case for the time being.
    // It should actually be possible to load a uniform in an accumulator,
    // not bothering right now.
    if (instr.isUniformLoad()) {
      continue;
    }

    // If 'instr' is not last usage of the found var, skip
    live.computeLiveOut(i, liveOut);  // Compute vars live-out of instr
    if (!(instr.src_a_regs().member(def) && !liveOut.member(def))) continue;

    // Can't remove this test.
    // Reason: There may be a preceding instruction which sets the var to be replaced.
    //         If 'prev' is conditional, replacing the var with an acc will ignore the previously set value.
    if (!prev.is_always()) {
/*
      std::string msg;
      msg << "peephole_1(): Skipping replacement for line " << i << " because prev.is_always() == false\n"
          << "prev : " << prev.dump()  << "\n"
          << "instr: " << instr.dump() << "\n";

      warning(msg);
*/
      continue;
    }


    //
    // Do the actual converions
    //
    Reg current(REG_A, def);
    Reg replace_with(ACC, instrs.get_free_acc(i - 1, i));
    assert(replace_with.regId != -1);

    prev.rename_dest(current, replace_with);
    renameUses(instr, current, replace_with);

    warn << "peephole1 conversion. Before:\n"
         << "  " << instrs[i-1].dump() << "\n"
         << "  " << instrs[i].dump()   << "\n"
         << "After: \n"
         << "  " << prev.dump()  << "\n"
         << "  " << instr.dump() << "\n"
    ;

    instrs[i-1] = prev;
    instrs[i]   = instr;

    // DANGEROUS! Do not use this value downstream.   
    // Currently stored for debug display purposes only! 
    allocated_vars[def].reg = replace_with;    

    subst_count++;
  }

  return subst_count;
}

/**
 * Replace assign-only variables with an accumulator
 */
int peephole_2(Liveness &live, Instr::List &instrs, RegUsage &allocated_vars) {
  int subst_count = 0;

  for (int i = 1; i < instrs.size(); i++) {
    Instr instr = instrs[i];
    if (!instr.has_registers()) continue;  // Doesn't help much

    // Guard for this special case for the time being.
    // It should actually be possible to load a uniform in an accumulator,
    // not bothering right now.
    if (instr.isUniformLoad()) {
      continue;
    }

    Reg dst = instr.dst_a_reg();
    if (dst.tag == NONE) continue;
    RegId def = dst.regId;

    if (!allocated_vars[def].only_assigned()) continue;

    Reg current(REG_A, def);
    Reg replace_with(ACC, instrs.get_free_acc(i, i));
    assert(replace_with.regId != -1);

    instr.rename_dest(current, replace_with);
    instrs[i] = instr;

    // DANGEROUS! Do not use this value downstream (remember why, old fart?).   
    // Currently stored for debug display purposes only! 
    allocated_vars[def].reg = replace_with;    

    subst_count++;
  }

  return subst_count;
}

}  // anon namespace


/**
 * @return true if any replacements were made, false otherwise
 */
bool combineImmediates(Liveness &live, Instr::List &instrs) {
  bool found_something = false;

  int const LAST_USE_LIMIT = 50;

  // Detect all LI instructions
  for (int i = 0; i < (int) instrs.size(); i++) {
    Instr &instr = instrs[i];
    if (instr.tag != InstrTag::LI) continue;

    if (instr.LI.imm.is_small_imm()) {
      auto const &reg_usage = live.reg_usage()[instr.dest().regId];

      if (reg_usage.assigned_once()) {
        assert(reg_usage.first_usage() == reg_usage.first_dst());
        bool can_remove = true;

        for (int j = reg_usage.first_usage() + 1; j <= reg_usage.last_usage(); j++) {
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
          instr.set_skip();
        }
      }

      continue;
    }

    // Scan forward to find replaceable LI's (i.e. LI's with same value in same or child block)
    int last_use = i;

    // Detect subsequent LI instructions loading the same value
    for (int j = i + 1; j < (int) instrs.size(); j++) {
      Instr &instr2 = instrs[j];

      if (instr2.is_branch()) {  // Don't go over branches, this affects liveness in a bad way
        break;
      }

      if (last_use + LAST_USE_LIMIT < j) {  // This is here for performance reasons, to avoid fully scanning
                                            // huge kernels.
        //debug("Hit last use limit, stopping forward scan");
        break;
      }



      if (instr2.is_dst_reg(instr.dest())) {
        break;
      }

      if (instr2.tag != InstrTag::LI) continue;
      if (instr2.LI.imm != instr.LI.imm) continue;
      if (!live.cfg().is_parent_block(j, live.cfg().block_at(i))) continue;

      //Log::debug << "Could replace LI at " << j << " (block " << live.cfg().block_at(j) << "): "
      //     << instr2.mnemonic(false) << "\n"
      //     << "  Scanning for dest reg: " << instr2.dest().dump() << "\n"
      //;

      //
      // Find and replace all occurences of the second LI with the first LI instruction
      //
      int num_subsitutions = 0;
      Reg current      = instr2.dest();
      Reg replace_with = instr.dest();

      // The bulk of the time (99%) in this function goes into the following part, 
      // last init + loop.
      //t3.start();

      // Limit search range to reg usage, or until end of block
      int last = live.cfg().block_end(j);
      {
        RegUsage &reg_usage = live.reg_usage();
        assert(!reg_usage[current.regId].unused());
        int last_usage = reg_usage[current.regId].last_usage();
       
        if (last > last_usage) last = last_usage;
      }

      for (int k = j + 1; k <= last; k++) {
        Instr &instr3 = instrs[k];
        if (!instr3.has_registers()) continue;

        if (instr3.is_dst_reg(current)) {
          break;  // Stop if var to replace is rewritten
        }


        Log::debug << "Renaming instr:\n"
                   << "current     : " << i << ": " << current.dump()         << "\n"
                   << "instr3      : " << k << ": " << instr3.mnemonic(false) << "\n"
                   << "replace_with:   "    << ": " << replace_with.dump()    << "\n"
        ;

        if (renameUses(instr3, current, replace_with)) {
          num_subsitutions++;
        }
      }
      //t3.stop();

      if (num_subsitutions > 0) {
        last_use = j;

        Log::debug << "Setting skip on instruction at " << j;
        instr2.set_skip();
      }
    }

    found_something = true;
  }

  return found_something;
}


/**
 * Optimisation passes that introduce accumulators
 *
 * @param allocated_vars write param; note which vars have an accumulator registered
 *
 * @return Number of substitutions performed;
 *
 * ============================================================================
 * NOTES
 * =====
 *
 * * It is possible that a variable gets used multiple times, and the last usage of it
 *   is replaced by an accumulator.
 *
 *   For this reason, it is dangerous to keep track of the substitutions in `allocated_vars`,
 *   and to ignore the variable replacement due to acc usage later on. There may still be instances
 *   of the variable that need replacing.
 */
int introduceAccum(Liveness &live, Instr::List &instrs) {
  RegUsage &allocated_vars = live.reg_usage();

#ifdef DEBUG
  for (int i = 0; i < (int) allocated_vars.size(); i++) {
    assert(allocated_vars[i].reg.tag == NONE);  // Safeguard for the time being
  }
#endif  // DEBUG

  int subst_count = 0;
  int const MAX_RANGE_SIZE = 15;  // 10 -> so that tmp var in sin_v3d() gets replaced

  // Picks up a lot usually, but range_size > 1 seldom results in something
  for (int range_size = 1; range_size <= MAX_RANGE_SIZE; range_size++) {
    int count = peephole_0(range_size, instrs, allocated_vars);

/*
    if (count > 0 && range_size > 1) {
      std::string msg = "peephole_0 for range size ";
      msg << range_size << ", num substs: " << count;
      debug(msg);
    }
*/

    subst_count += count;
  }


  // This peephole still does a lot of useful stuff
  {
    int count = peephole_1(live, instrs, allocated_vars);
    // debug << "peephole_1, num substs: " << count;

    subst_count += count;
  }

  // And some things still get done with this peephole, regularly 1 or 2 per compile
  {
    int count = peephole_2(live, instrs, allocated_vars);
/*
    if (count > 0) {
      debug  << "peephole_2, num substs: " << count;
    }
*/

    subst_count += count;
  }

  return subst_count;
}

}  // namespace V3DLib
