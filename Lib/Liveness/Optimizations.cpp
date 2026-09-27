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

  for (int i = item.first_usage(); i <= item.last_usage(); i++) {
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

    // Check instructions for unused accumulator
    int acc_id = instrs.get_free_acc(item.first_usage(), item.last_usage());
    if (acc_id == -1) {
      warn << "peephole_0: All accumulators used";
      continue;
    }
/*
    if (acc_id >= 0) {
      warn << "peephole_0 var_id: " << var_id      << ", "
           << "acc_id: "            << acc_id      << ", "
           << "item: "              << item.dump();

    }
*/
    // Check if the given ACC has not been assigned in the meantime
    if (allocated_vars.check_overlap_usage(Reg(ACC, acc_id), item)) {
      warn << "acc_id: " << acc_id << " already in use, can't assign";
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
    // not bothering right now.
    if (instr.isUniformLoad()) {
      continue;
    }

    live.computeLiveOut(i, liveOut);  // Compute vars live-out of instr

    // If 'instr' is not last usage of the found var, skip
    if (!(instr.src_a_regs().member(def) && !liveOut.member(def))) continue;

    // Can't remove this test.
    // Reason: There may be a preceding instruction which sets the var to be replaced.
    //         If 'prev' is conditional, replacing the var with an acc will ignore the previously set value.
    if (!prev.is_always()) {
      continue;
    }

    Reg current(REG_A, def);
    Reg replace_with(ACC, instrs.get_free_acc(i - 1, i));
    assert(replace_with.regId != -1);

    prev.rename_dest(current, replace_with);
    renameUses(instr, current, replace_with);
    instrs[i-1] = prev;
    instrs[i]   = instr;
/*
    warn << "peephole_1 post:\n"
         << "  " << instrs[i-1].dump()
         << "  " << instrs[i].dump();
*/
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

      if (instr.dest().is_special()) {
        info << "combineImmediates special dest register, not combinining, instr: " << instr.dump();
        continue;
      }

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
          info << "combineImmediates can_remove, instr: " << instr.dump();
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
                   << "replace_with:   "    << ": " << replace_with.dump()    << "\n";

        if (renameUses(instr3, current, replace_with)) {
          num_subsitutions++;
        }
      }

      if (num_subsitutions > 0) {
        last_use = j;
        Log::debug << "Setting skip on instruction at " << j;

        instrs.set_skip(j);
      }
    }

    found_something = true;
  }

  return found_something;
}


/**
 * @brief Optimisation passes that introduce accumulators
 *
 * @param allocated_vars write param; note which vars have an accumulator registered
 * @return               Number of substitutions performed
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
    auto &item = allocated_vars[i];

    //
    // Paranoia safeguards
    //

    //reg's should not be allocated already
    assert(item.reg.tag == NONE);

    // Single range has only a dst register set
    if (item.use_range() == 1) {
      assert(item.assigned_once());
      //warn << "range 1 " << i << ": " << item.dump();
    }

/*
    if (item.unused()) {
      warn << "unused " << i << ": " << item.dump();
    }
*/    
  }
#endif // DEBUG

  int subst_count = 0;

  // Should be >= 2 for any effective use
  // >= 10 so that tmp var in sin_v3d() gets replaced
  int const MAX_RANGE_SIZE = 4; //= 8; //= 15;

  //
  // Picks up a lot usually
  //
  for (int range_size = 1; range_size <= MAX_RANGE_SIZE; range_size++) {
    int count = peephole_0(range_size, instrs, allocated_vars);

    if (count > 0) {
      warn << "peephole_0 range_size: " << range_size << ", " << count << " substitutions";
    } 

    subst_count += count;
  }

//#ifdef DEBUG
#if 0
  for (int i = 0; i < instrs.size(); ++i) {
    auto const &instr = instrs[i];
    uint32_t acc_mask = instr.get_acc_usage();  // Remember, get_acc_usage() returns *used* acc's
    if (acc_mask != 0) {
      warn << "peephole_0 mask: " << acc_mask << ", instr " << i << ": " << trim_s(instr.dump());
    }
  }
#endif


#if 0
  // This peephole still does useful stuff
  // Plenty of substitutions when peephole_0 disabled.
  {
    int count = peephole_1(live, instrs, allocated_vars);

    if (count > 0) {
      warn << "peephole_1: " << count << " substitutions";
    } 

    subst_count += count;
  }


  // And some things still get done with this peephole, regularly 1 or 2 per compile
  {
    int count = peephole_2(live, instrs, allocated_vars);

    if (count > 0) {
      warn << "peephole_2: " << count << " substitutions";
    } 

    subst_count += count;
  }
#endif

  return subst_count;
}

}  // namespace V3DLib
