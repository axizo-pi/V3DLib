///////////////////////////////////////////////////////////////////////////////
// Liveness Analysis
//
///////////////////////////////////////////////////////////////////////////////
#include "Liveness.h"
#include <iostream>
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Target/Subst.h"
#include "Common/CompileData.h"
#include "Optimizations.h"
#include "UseDef.h"
#include "Support/Helpers.h"  // contains()
#include "Support/Timer.h"

namespace V3DLib {
namespace {

int count_skips(Instr::List &instrs) {
  int ret = 0;

  for (int i = 0; i < (int) instrs.size(); i++) {
    if (instrs[i].skip()) {
      ret++;
    }
  }

  return ret;
}


/**
 * @brief Replace the variables with the assigned registers for the given instruction
 *
 * This functions assigns real registers to the 'variable registers' of the instruction.
 *
 * The incoming instructions all have REG_A as registers but signify variables.
 * The reg id's indicate the variable at this stage.
 *
 * Allocation is first done with reg types TMP_A/TMP_B,
 * to avoid accidental replacements of registers with same id.
 * This has happened IRL.
 */
void allocate_registers(Instr &instr, RegUsage const &alloc) {

  auto check_regfile_register = [&instr] (Reg const &replace_with, RegId r) -> bool {
    if (replace_with.tag == REG_A) return true;
    if (Platform::compiling_for_vc4() && replace_with.tag == REG_B) return true;

    UseDefReg out(instr);

    std::string msg = "allocate_registers(): allocated register must be in register file.";
    msg << "\n"
        << "Instruction: " << instr.dump() << ", "
        << "Registers: " << out.dump() << ", "
        << "Reg id : " << r << ", alloc value: " << replace_with.dump();

    cerr << msg << thrw;

    return false;
  };

  UseDef useDefSet(instr, false, false);  // Registers only usage REG_A

  if (useDefSet.def.tag != NONE) {
    RegId r = useDefSet.def.regId; 
    assert(!alloc.get(r).unused());
    Reg replace_with = alloc.get(r).reg;

    if (check_regfile_register(replace_with, r)) {
      replace_with.tag = (replace_with.tag == REG_A)?TMP_A:TMP_B;
      instr.rename_dest(Reg(REG_A, r), replace_with);
    }
  }

  for (auto r: useDefSet.use) {
    assert(!alloc.get(r).unused());
    Reg replace_with = alloc.get(r).reg;

    if (!check_regfile_register(replace_with, r)) continue;
    replace_with.tag = (replace_with.tag == REG_A)?TMP_A:TMP_B;

    renameUses(instr, Reg(REG_A, r), replace_with);
  }

  substRegTag(&instr, TMP_A, REG_A);
  substRegTag(&instr, TMP_B, REG_B);
}


/**
 * Removes all SKIP instructions from the list
 *
 * NOTE: Creating a new list is MUCH faster than inline removal using Instr::remove().
 * i.e.  Remove 1857 SKIPs from kernel final size 140828
 *        - remove() -> 28.557023s
 *        - new list -> 0.170661s
 */
Instr::List remove_skips(Instr::List &instrs) {
  Instr::List ret;

  int cur   = 0;
  int count = 0;

  while (cur < instrs.size()) {
    if (instrs[cur].skip()) {
      count++;
    } else {
      ret << instrs[cur];
    }
    cur++;
  }

  return ret;
}

}  // anon namespace


///////////////////////////////////////////////////////////////////////////////
// Class Liveness
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Determine the liveness sets for each instruction.
 */
void Liveness::compute_liveness(Instr::List const &instrs) {
  // Initialise live mapping to have one entry per instruction
  setSize(instrs.size());

  // For temporarily storing live-in and live-out variables
  RegIdSet liveIn;
  RegIdSet liveOut;

  bool changed = true;
  int count = 0;

  // Iterate until no change, i.e. fixed point
  while (changed) {
    changed = false;

    // Propagate live variables backwards
    for (int i = instrs.size() - 1; i >= 0; i--) {
      // Instructions in liveness MUST be consecutive, don't skip any
      auto const &instr = instrs[i];

      bool also_set_used = false;

      if (/* instr.has_registers() && */ instr.isCondAssign()) {  // no performance impact ~ 1.5%
        Reg dst = instr.dst_a_reg();

        if (dst.tag != NONE) {
          auto &item = m_reg_usage.get(dst.regId);

          // If the dst variable is not used before, it should not be set as used as well
          int first = item.first_dst();
          assert(first != -1 && first <= i);
          also_set_used = (first < i);
        }
      }

      // Compute 'use' and 'def' sets
      UseDef useDef(instr, false, also_set_used);
      //warn << "useDef " << i << ": " <<  useDef.dump() << "instr: " << instr.mnemonic();

      computeLiveOut(i, liveOut);
      //warn << "liveOut computeLiveOut post " << i << ": " <<  liveOut.dump() << ", instr: " << instr.mnemonic();

      liveIn = liveOut;
      if (useDef.def.tag != NONE) {
        liveIn.remove(useDef.def.regId);  // Remove the 'def' set from the live-out set to give live-in set
      }
      liveIn.add(useDef.use);
      //warn << "liveIn " << i << ": " <<  liveIn.dump() << ", instr: " << instr.mnemonic();

      if (insert(i, liveIn)) {
        changed = true;
      }
    }

    count++;
  }

  info << "compute_liveness " << count << " iterations.";
}


void Liveness::clear() {
  m_cfg.clear();
  m_set.clear();
  m_reg_usage.reset();
}


void Liveness::compute(Instr::List const &instrs, bool do_accumulators) {
  clear();

  m_cfg.build(instrs);
  m_reg_usage.set_used(instrs, do_accumulators);

  // Don't bother with liveness for accumulators, it is useless
  if (do_accumulators) return;

  compute_liveness(instrs); // performance hog 23/28s
  assert(instrs.size() == size());
  m_reg_usage.set_live(*this);
  m_reg_usage.check();

#ifdef OUTPUT_COMPILEDATA
  // Compile data only outputted for full liveness (not acc's)
  compile_data.reg_usage_dump = m_reg_usage.dump();
  compile_data.liveness_dump = dump();
#endif // OUTPUT_COMPILEDATA
}


/**
 * @brief Compute live-out sets for each instruction.
 *
 * Compute the live-out variables of an instruction, given the live-in
 * variables of all instructions and the CFG.
 */
void Liveness::computeLiveOut(InstrId i, RegIdSet &liveOut) {
  liveOut.clear();

  for (auto const &val : m_cfg[i]) {
    //warn << "computeLiveOut " << i << ": val: " << val;
    liveOut.add(get(val));
  }
}


void Liveness::setSize(int size) {
  m_set.resize(size);
}


/**
 * @return true if something inserted, false otherwise
 */
bool Liveness::insert(int index, RegIdSet const &set) {
  auto &item = m_set[index];

  int prev_size = (int) item.size();
  item.add(set);
  return ((int) item.size() != prev_size);
}


std::string Liveness::dump() {
  std::string ret;

  for (int i = 0; i < (int) m_set.size(); ++i) {
    auto &item = m_set[i];

    std::string line;
    line << i << ": (" << item.size() << ") ";

    bool did_first = false;

    for (auto it : item) {
      if (did_first) {
        line << ", ";
      } else {
        did_first = true;
      }
      line << it;
    }

    ret << line << "\n";
  }

  if (ret.empty()) ret += "<Empty>";

  ret += "\n";
  return ret;
}


/**
 * @brief Introduce optimizations where possible in the instruction list
 *
 * This is done before the actual liveness analysis.
 * The idea is to minimize beforehand the number of variables considered
 * in the liveness analysis.
 */
void Liveness::optimize(Instr::List &instrs, int numVars) {
  assertq(count_skips(instrs) == 0, "optimize(): SKIPs detected in instruction list");
  
  Liveness live(numVars);
  live.compute(instrs);

  if (combineImmediates(live, instrs)) {
    info << "instructions have changed, redo liveness";
    live.compute(instrs);
  }

#ifdef OUTPUT_COMPILEDATA
  compile_data.target_code_after_immediates = instrs.dump();
#endif // OUTPUT_COMPILEDATA

  //
  // vc7 has no general purpose accumulators, don't bother replacing variables with them
  //
  if (!Platform::compiling_for_vc7()) {
    int prev_count_skips = count_skips(instrs);

#ifdef OUTPUT_COMPILEDATA
    compile_data.num_accs_introduced =
#endif // OUTPUT_COMPILEDATA
    introduceAccum(live, instrs);

    assertq(prev_count_skips == count_skips(instrs), "SKIP count changed after introduceAccum()");
  }

  // Times for following (now) insignificant
  instrs = remove_skips(instrs);
  assertq(count_skips(instrs) == 0, "optimize(): SKIPs detected in instruction list after cleanup");

#ifdef OUTPUT_COMPILEDATA
  compile_data.target_code_before_liveness = instrs.dump();
#endif // OUTPUT_COMPILEDATA
}


/**
 * @brief Return index of accumulator which is free for the given
 *        range in the instruction list.
 *
 * @return Index of first free accumulator, -1 if none found.
 *
 * ================================================================
 * Notes
 * -----
 *
 * 1. From VC4 Architecture Guide:
 *   - p.18:
 *     r4 (acc4): Receives data from most of the closely coupled hardware units (notably SFU, TMU read).
 *                r4 is a read only register from the processors perspective.
 *     r5 (acc5): Used for fragment shading and can not be used as a general purpose register.
 *
 *   - p.28:
 *     "The accumulators r4 and r5 have special functions and cannot be used
 *     as general-purpose accumulator registers."
 */
int get_free_acc(Instr::List const &instrs, Range const &use_range) {
  assert(use_range.last() < instrs.size());
  timers.start("get_free_acc(Range)");

  uint32_t acc_use = 0xffffffff;  // Keeps track of free acc's, default all free

  for (int i = use_range.first(); i <= use_range.last(); ++i) {
    auto const &instr = instrs[i];

    uint32_t acc_mask = instr.get_acc_usage();  // Remember, get_acc_usage() returns *used* acc's
    acc_use = acc_use & ~acc_mask;
  }

  // Also masks out unused bits. See Note 1.
  //
  if (Platform::compiling_for_vc4()) {
    acc_use = acc_use & 0xf;   // r0-r3
  } else {
    // vc6 all acc's appear to be available
    acc_use = acc_use & 0x3f;  // r0-r5
  }

  // Determine first non-zero bit
  int ret = -1;

  for (int i = 0; i < 5; ++i) {
    if ((acc_use & (1 << i)) != 0) {
      ret = i;
      break;
    }
  }

  timers.stop("get_free_acc(Range)");
  return ret;
}


Reg get_free_acc(Instr::List const &instrs, int line_number, Liveness const &live) {
  //warn << "Called ::get_free_acc(), line: " << line_number;
  assert(0 <= line_number && line_number < instrs.size());
  timers.start("::get_free_acc");

  auto const &instr = instrs[line_number];
/*
  warn << "Current "
       << "dest: " << instr.dest().dump() << ", "
       << "src's: ("
       << instr.src_a_reg().dump() << ", "
       << instr.src_b_reg().dump() << "), "
       << "instr: "
       << instr.mnemonic(false);
*/

  RegUsage const &allocated_vars = live.reg_usage();
  //warn << "reg_usage:\n" << allocated_vars.dump(true);

  int acc_id = allocated_vars.dst_range(line_number);
  assertq(acc_id >= 0, "::get_free_acc no accumulators available");

  Reg ret(ACC, acc_id);

  timers.stop("::get_free_acc");
  return ret;
}


void allocate_registers(Instr::List &instrs, RegUsage const &alloc) {
  for (int i = 0; i < instrs.size(); i++) {
    allocate_registers(instrs.get(i), alloc);
  }
}

}  // namespace V3DLib
