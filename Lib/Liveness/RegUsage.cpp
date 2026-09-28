#include "RegUsage.h"
#include "Support/basics.h"
#include "Support/Platform.h"  // size_regfile()
#include "Liveness.h"
#include "UseDef.h"

namespace V3DLib {

using namespace Target;

namespace {

std::string get_unused_list(RegUsage const &alloc_list) {
  std::string ret;

  for (int i = 0; i < (int) alloc_list.size(); i++) {
    if (alloc_list[i].unused()) {
      ret << i << ",";
    }
  }

  return ret;
}


std::string get_never_assigned_list(RegUsage const &alloc_list) {
  std::string ret;

  for (int i = 0; i < (int) alloc_list.size(); i++) {
    if (alloc_list[i].never_assigned()) {
      ret << i << ",";
    }
  }

  return ret;
}


std::string get_assigned_only_list(RegUsage const &alloc_list) {
  std::string ret;

  // NOTE: var 0 (QPU id) skipped, this always gets set because passed in as uniform,
  //       but often does not get used.

  for (int i = 1; i < (int) alloc_list.size(); i++) {
    if (alloc_list[i].only_assigned()) {
      ret << i << ",";
    }
  }

  return ret;
}

}  // anon namespace


///////////////////////////////////////////////////////////////////////////////
// Class RegUsageItem
///////////////////////////////////////////////////////////////////////////////

bool RegUsageItem::unused() const {
  return (m_use_dst.empty() && m_src_range.count() == 0);
}


bool RegUsageItem::assigned_once() const {
  assert(!unused());
  return m_use_dst.size() == 1;
}


std::string RegUsageItem::dump() const {
  std::string ret;

  if (unused()) {
    ret << "Not used";
    return ret;
  }

  std::string dst_list;
  for (int i = 0; i < (int) m_use_dst.size(); ++i) {
    if (i != 0) {
      dst_list << ", ";
    }
    dst_list << m_use_dst[i];
  }

  ret << reg.dump() << "; "
      << "src(" << m_src_range.dump() << "); "
      << "dst: {" << dst_list << "}; "
      << "live(" << m_live_range.dump() << ")";

  return ret;
}


void RegUsageItem::add_dst(int n, bool is_cond_assign) {
  assertq(m_use_dst.empty() || m_use_dst.back() < n, "RegUsageItem::add_dst() failed");

  m_use_dst << n;
}


void RegUsageItem::add_src(int n)  { m_src_range.add(n); }
void RegUsageItem::add_live(int n) { m_live_range.add(n); }


/**
 * Number of instructions over which this variable is live.
 *
 * Note that it is possible (in theory only, I hope), that there
 * are gaps of liveness possible within this range.
 * This could happen if the variable assigned to within this range.
 */
int RegUsageItem::live_range() const {
  return m_live_range.range();
}


/**
 * @brief Get number of instructions from first assignment till last usage, inclusive.
 *
 * Following is mostly, _but not always_, true:
 *
 *     assertq(m_use_dst[0] + 1 == m_live_range.first(), "dst does not match live range", true);
 */
int RegUsageItem::use_range() const {
  if (unused()) return 0;

  if (m_live_range.empty()) {
    assertq(m_use_dst.size() == 1, "Live range empty, multiple dst's", true);
    return 1;
  }

  assertq(!m_use_dst.empty(), "No dst's", true);

  int first = m_use_dst[0];
  int last  = m_live_range.last();
  int ret   = (last - first + 1);
  assert(ret > 0);
  return ret;
}


/**
 * @return first dst if present, -1 otherwise
 */
int RegUsageItem::first_dst() const {
  if (m_use_dst.empty()) return -1;
  return m_use_dst[0];
}


/**
 * @brief Return the first line in which this variable is used (either as src or dst)
 */
int RegUsageItem::first_usage() const {
  assert(!m_use_dst.empty());
  return m_use_dst[0];         // This assumes that first dst is lowest number
}


/**
 * Get last line number for which variable is used (either as src or dst)
 */
int RegUsageItem::last_usage() const {
  assert(m_src_range.first() == -1 || m_src_range.first() >= first_dst());

  if (only_assigned())       return first_dst();
  if (!m_live_range.empty()) return m_live_range.last();
  if (!m_src_range.empty())  return m_src_range.last();

  return -1;
}


/**
 * @return true if ranges overlap, false otherwise.
 */
bool RegUsageItem::use_overlaps(RegUsageItem const &rhs) const {
  if (first_usage() > rhs.last_usage()) {
    return false;
  }

  if (first_usage() < rhs.first_usage()) {
    return last_usage() > rhs.first_usage();
  }

  warn << "use_overlaps lhs: " << dump() << ", rhs: " << rhs.dump();

  // All other cases overlap
  assert(first_usage() >= rhs.first_usage() && first_usage() <= rhs.last_usage()); 
  return true;
}


void RegUsageItem::reset() {
  reg.tag = NONE;
  m_src_range.reset();
  m_use_dst.clear();
  m_live_range.reset();
}


bool RegUsageItem::empty() const {
  return ( reg.tag == NONE
        && m_src_range.empty()
        && m_use_dst.empty()
        && m_live_range.empty()
  );
}

///////////////////////////////////////////////////////////////////////////////
// Class RegUsage
///////////////////////////////////////////////////////////////////////////////

RegUsage::RegUsage(int numVars) : Parent(numVars) {
  reset();
}


void RegUsage::reset() {
  assert(size() > 0);

  for (auto &a : *this) {
    a.reset();
  }
}


RegUsageItem &RegUsage::get(int i) {
  if (i > (int) size()) {
    Log::warn << "RegUsage::get(): resizing from " << (int) size() << " to " << (i + 1);
    resize(i + 1);
  }

#ifdef DEBUG
  // at() is useful because it does bounds checking,
  // which is also the reason it is inefficient
  return at(i);
#else
  return (*this)[i];
#endif
}

RegUsageItem const &RegUsage::get(int i) const {
  assert(i < (int) size());
  return (*this)[i];
}


void RegUsage::set_used(Instr::List const &instrs, bool do_accumulators) {
#ifdef DEBUG
  //Log::warn << "RegUsage.set_used() size: " << (int) size();

  for (auto &a : *this) {
    assert(a.empty());
  }
#endif

  for (int i = 0; i < instrs.size(); i++) {
    if (!instrs[i].has_registers()) continue;

    UseDef out(instrs[i], do_accumulators, false);
    //warn << "set_used out " << i << ": " << out.dump();

    if (out.def.tag != NONE) {
      //warn << "add_dst: " << i;
      assert(out.def.regId < (int) size());

      auto &item = get(out.def.regId);
      item.add_dst(i, instrs[i].isCondAssign());
    }

    for (auto r : out.use) {
      //warn << "add_src: " << i;
      assert(r < (int) size());

      auto &item = get(r);
      item.add_src(i);
      //warn << "add_src item: " << item.dump();
    }
  }
}


void RegUsage::set_live(Liveness &live) {
  for (int i = 0; i < live.size(); i++) {
    auto &item = live[i];  // item holds list of accumulator indexes.
    //warn << "set_live " << i << ": " << item.dump();

    for (auto it : item) {
      auto &item2 = (*this)[it];
      //warn << "item2: " << item2.dump();

      item2.add_live(i);
    }
  }
}


/**
 * Check internal consistency of used variables
 *
 * If anything is detected here, it is a compile error.
 *
 * ===================================================
 *
 * - Case 'instruction variables which are assigned but never used'
 *   is pretty common and not much of an issue any more.
 *   E.g. It occurs if condition flags need to be set and the result of the 
 *   operation is discarded.  
 *   In all honesty, it would be better to write to the NOP register in this case (TODO).
 *
 *   Does not need to be tested.
 */
void RegUsage::check() const {
  std::string ret;

  {
    std::string tmp = get_never_assigned_list(*this);
    if (!tmp.empty()) {
      std::string msg;
      msg << "  There are internal instruction variables which are used but never assigned.\n"
          << "  Variables: " << tmp << "\n";

      ret << msg;
    }
  }

  {
    std::string tmp;

    for (int i = 0; i < (int) size(); i++) {
      auto const &item = (*this)[i];
      if (!item.regular_use())   continue;
      if (item.never_assigned()) continue;  // Tested in previous block

      if (item.first_live() <= item.first_dst()) {
        tmp << "  Variable " << i << " is live before first assignment" << "\n";
      }
    }

    if (!tmp.empty()) {
      tmp << "\n  This can happen if the first assignment is in a conditional block (If, Where, For etc).\n";
      ret << tmp;
    }
  }
 
  if (!ret.empty()) {
    std::string prefix = "RegUsage internal error(s) ";
    if (Platform::compiling_for_vc4()) {
      prefix << "vc4";
    } else {
      prefix << "v3d";
    }
    prefix << ":\n";

    cerr << (prefix + ret) << thrw;
  } 
}


std::string RegUsage::allocated_registers_dump() const {
  std::string ret;

  for (int i = 0; i < (int) size(); i++) {
    ret << i << ": " << (*this)[i].reg.dump() << "\n";
  }

  return ret;
}


std::string RegUsage::dump(bool verbose) const {
  if (empty()) return "<Empty>";

  if (!verbose) return allocated_registers_dump();

  bool const ShowUnused = false;

  std::string ret;

  for (int i = 0; i < (int) size(); i++) {
    auto const &item = (*this)[i];

    if (ShowUnused || !item.unused()) {
      ret << i << ": " << item.dump() << "\n";
    }
  }

  std::string tmp = get_unused_list(*this);
  if (!tmp.empty()) {
    ret << "\nNot used: " << tmp << "\n";
  }

  tmp = get_assigned_only_list(*this);
  if (!tmp.empty()) {
    ret << "\nOnly assigned: " << tmp << "\n";
  }

  tmp = get_never_assigned_list(*this);
  if (!tmp.empty()) {
    ret << "\nNever assigned: " << tmp << "\n";
  }

  return ret;
}


std::string RegUsage::dump_use_ranges() const {
  std::string ret;

  Seq<int> ranges;

  for (int i = 0; i < (int) size(); ++i) {
    ranges.append((*this)[i].use_range());
  }


  for (int i = 0; i < ranges.size(); ++i) {
    ret << ranges[i] << ", ";
  }

  return ret;
}


/**
 * @brief Check if found acc does not conflict with other uses of this acc.
 *
 * It may have been assigned to another var whose use-range overlaps with current var.
 *
 * @return true if overlap detected, false otherwise
 */
bool RegUsage::check_overlap_usage(Reg acc, RegUsageItem const &item) const {
  //warn << "check_overlap_usage checking " << acc.dump();
  assert(acc.tag == ACC);
  assert(acc.regId >= 0);

  for (int i = 0; i < (int) size(); ++i) {
    auto const &cur = (*this)[i];
    if (cur.reg != acc) continue;
    //warn << "Same ACC: " << cur.reg.dump();

    if (cur.use_overlaps(item)) {
      warn << "check_overlap_usage: Detected conflicting usage of replacement acc";
      return true;
    }
  }

  return false;
}


/**
 * @brief Determine dst usage range per accumulator.
 *
 * Find the largest dst before instruction and lowest after.
 *
 * **TODO:** To do it properly, you need the explicit src lines as well.
 *
 * @return index of (lowest) available accumulator if found, -1 otherwise.
 */
int RegUsage::dst_range(int line_number) const {
  //info << "Called dst_range line_number: " << line_number;

  int first_unused_acc = -1;

  // This loop checks dst's only! Should really be checking src's as well (TODO)
  for (int i = 0; i < (int) size(); ++i) {
    auto const &item = get(i);
    if (item.unused()) {
      first_unused_acc = i;  // Found available acc
      break;
    }

    auto &use_dst = item.use_dst();
    if (use_dst.empty()) continue;

    int bottom_dst = -1;
    int top_dst = -1;

    for (int j = 0; j < (int) use_dst.size(); ++j) {
      int val = use_dst[j];

      if (val <= line_number && (bottom_dst == -1 || val > bottom_dst)) {
        bottom_dst = val;
      }

      if (val > line_number && (top_dst == -1 || val < top_dst)) {
        top_dst = val;
      }
    }

    bool available = true;

    if (bottom_dst == -1) {
      // This is is fine; no assignment to current acc before this line.
      // It is free for use.
    } else if (top_dst != -1) {
      available = false;
    } else if (top_dst == -1) {
      // Check src range to see if acc is valid; this can be done better.
      // This is a patch-up job; better would be to test explicit src line numbers (TODO)
      if (item.src_range().last() >= line_number) {
        available = false;
      }
    } else {
      // Not expecting this to be ever called again,
      // but we're paranoid so keeping it in.
      std::string msg = "RegUsage::dst_range(): Handle this case when encountered";

      warn << msg << "\n  "
           << "acc" << i << " "
           << "bottom: " << bottom_dst << ", "
           << "top: "    << top_dst    << "\n  "
           << "item: "   << item.dump();

      assertq(false, msg);
    }

    if (available) {
      // Found available accumulator
      info << "dst_range: acc" << i << " available";
      first_unused_acc = i;
      break;
    }
  }

  if (first_unused_acc > 4) {
    warn << "dst_range blocking special accumulator acc5 for now.";
    first_unused_acc = -1;
  }

  if (first_unused_acc > 3) {
    warn << "dst_range returning special accumulator acc4 or acc5; check for conflicts.";
  }

  return first_unused_acc;
}

}  // namespace V3DLib
