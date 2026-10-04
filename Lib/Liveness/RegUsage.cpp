#include "RegUsage.h"
#include "Support/basics.h"
#include "Support/Platform.h"  // size_regfile()
#include "Liveness.h"
#include "UseDef.h"
#include <algorithm>           // std::sort()

namespace V3DLib {

using namespace Target;

namespace {

MAYBE_UNUSED std::string get_unused_list(RegUsage const &alloc_list) {
  std::string ret;

  for (int i = 0; i < (int) alloc_list.size(); i++) {
    if (alloc_list.get(i).unused()) {
      ret << i << ",";
    }
  }

  return ret;
}


std::string get_never_assigned_list(RegUsage const &alloc_list) {
  std::string ret;

  for (int i = 0; i < (int) alloc_list.size(); i++) {
    if (alloc_list.get(i).never_assigned()) {
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
    if (alloc_list.get(i).only_assigned()) {
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
  return (m_use_dst.empty() && m_use_src.size() == 0);
}


bool RegUsageItem::only_assigned() const  {
  bool ret =!m_use_dst.empty() && m_use_src.size() == 0;
  if (ret) {
		//warn << "only_assigned: " << dump();
    assert(m_live_range.empty());
  }

  return ret;
}


bool RegUsageItem::assigned_once() const {
  assert(!unused());
  return m_use_dst.size() == 1;
}


std::string RegUsageItem::vec_dump(std::vector<int> const &vec) const {
  std::string ret;
  ret << "{";

  for (int i = 0; i < (int) vec.size(); ++i) {
    if (i != 0) {
      ret << ", ";
    }

    ret << vec[i];
  }

  ret << "}";

  return ret;
}


std::string RegUsageItem::dump() const {
  if (unused()) return "Not used";

  std::string ret;
  ret << reg.dump() << "; "
      << "src: " << vec_dump(use_src()) << "; "
      << "dst: " << vec_dump(m_use_dst) << "; "
      << "live(" << m_live_range.dump() << ")";

  return ret;
}


void RegUsageItem::add_dst(int n) {
  // input values expected to be monotonic
  assertq(m_use_dst.empty() || m_use_dst.back() < n, "RegUsageItem::add_dst() failed");
  m_use_dst << n;
}


/**
 * **NOTE:** src input values are not monotonic.
 *           This is resolved in a lazy read: if `m_src_sorted == false`,
 *           redo sort (and uniqueness) on read.
 */
void RegUsageItem::add_src(int n) {
  m_use_src << n;
  m_src_sorted = false;
}


std::vector<int> const &RegUsageItem::use_src() const {
  if (!m_src_sorted) {
    std::sort(m_use_src.begin(), m_use_src.end());
    m_use_src.erase(unique(m_use_src.begin(), m_use_src.end()), m_use_src.end());
    m_src_sorted = true;
  }

  return m_use_src;
}


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
 */
int RegUsageItem::use_range() const {
  if (unused()) return 0;

  if (m_live_range.empty()) {
/*
    // Canary in introduceAccum()
    if (m_use_dst.size() > 1) {
      // see class Note 1.
      info << "use_range live range empty, multiple dst's: " << dump();
    }
*/

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

  if (m_live_range.empty() && m_use_dst.size() > 1) {
    //info << "first_usage live range empty, multiple dst's";
    return m_use_dst.back(); // This is a lie; see class Note 1.
  }

  return m_use_dst[0];         // This assumes that first dst is lowest number
}


/**
 * Get last line number for which variable is used (either as src or dst)
 */
int RegUsageItem::last_usage() const {
  assert(m_use_src.empty() || m_use_src.front() >= first_dst());

  if (only_assigned()) {
    // In this case this is the correct response; see class Note 1 anyway.
    return m_use_dst.back();
  }

  if (!m_live_range.empty()) return m_live_range.last();
  if (!m_use_src.empty())  return m_use_src.back();

  return -1;
}


Range RegUsageItem::usage() const {
  return Range(first_usage(), last_usage());
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

  // All other cases overlap
  //info << "use_overlaps lhs: " << dump() << ", rhs: " << rhs.dump();
  assert(first_usage() >= rhs.first_usage() && first_usage() <= rhs.last_usage()); 
  return true;
}


void RegUsageItem::reset() {
  reg.tag = NONE;
  m_src_sorted = true;
  m_use_src.clear();
  m_use_dst.clear();
  m_live_range.reset();
}


bool RegUsageItem::empty() const {
  return ( reg.tag == NONE
        && m_use_dst.empty()
        && m_live_range.empty()
  );
}


/**
 * @brief Get closest range for current item to line number
 *
 * Find the highest dst _before_ `line_number`
 * and highest src _before_ lowest dst after `line_number`.
 *
 * The bottom dst is _always_ below the line, the top of the range _may_ be below line.
 */
Range RegUsageItem::dst_range(int line_number) const {
  Range ret;

  if (m_use_dst.empty()) return ret;

  //
  // Determine dst's encompassing `line_number`
  //
  int bottom_dst = -1;
  int top_dst = -1;

  for (int j = 0; j < (int) m_use_dst.size(); ++j) {
    int val = m_use_dst[j];

    if (val <= line_number && (bottom_dst == -1 || val > bottom_dst)) {
      bottom_dst = val;
    }

    if (val > line_number && (top_dst == -1 || val < top_dst)) {
      top_dst = val;
    }
  }

  //
  // Determine highest src _below_ top dst
  //
  int top_src = -1;

  if (!use_src().empty()) {
    if (top_dst == -1) {
      // Just grab the highest src
      top_src = use_src().back();
    } else {
      for (int j = 0; j < (int) use_src().size(); ++j) {
        int val = use_src()[j];

        // NOTE: read reg can be in same operation as write, hence `<=`
        if (val > top_src && (top_src == -1 || val <= top_dst)) {
          top_src = val;
        }
      }
    }
  }

  if (top_src < bottom_dst) { // Also handles top_src == -1
    // If no read after bottom write, range is only on write
    top_dst = bottom_dst;
  } else {
    top_dst = top_src;
  }


  if (bottom_dst == -1) {
    if (top_src != -1 && top_src < line_number) {
      warn << "Edge case: last read is without preceding write and before line_number";
      return Range();
    }

    // If no bottom write, the first write is after the line.
    // This is perfectly safe.
    assert(top_dst == -1 || top_dst > line_number); // Paranoia
    return Range();
  }

  ret.unsafe_assign(bottom_dst, top_dst);
  return ret;
}


/**
 * @return true if current var in use at given line number, false otherwise
 */
bool RegUsageItem::in_use(int line_number) const {
  Range range = dst_range(line_number);

  if (range.in(line_number)) {
    return true;
  } else {
    //warn << "in_use line: " << line_number << ", OUTSIDE range: " << range.dump();
    return false;
  }
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


/**
 * TODO: Consider renaming to `operator[]`.
 */
RegUsageItem &RegUsage::get(int i) {
  if (i >= (int) size()) {
    Log::warn << "RegUsage::get(): resizing from " << (int) size() << " to " << (i + 1);
    resize(i + 1);
  }

  assertq(0 <= i && i < (int) size(), "RegUsage::get() index out of range", true);

#ifdef DEBUG
  // at() is useful because it does bounds checking,
  // which is also the reason it is inefficient
  return at(i);
#else
  return (*this)[i];
#endif
}


RegUsageItem const &RegUsage::get(int i) const {
  assertq(0 <= i && i < (int) size(), "RegUsage::get() index out of range", true);
  return (*this)[i];
}


bool RegUsage::empty() const {
  for (auto &a : *this) {
    if (!a.empty()) return false;
  }

  return true;
}


void RegUsage::set_used(Instr::List const &instrs, bool do_accumulators) {
  assert(empty());

  for (int i = 0; i < instrs.size(); i++) {
    auto const &instr = instrs[i];

    if (!instr.has_registers()) continue;

    UseDef out(instr, do_accumulators, false);

    MAYBE_UNUSED bool added = false;

    if (out.def.tag != NONE) {
      assert(out.def.regId < (int) size());
      auto &item = get(out.def.regId);
      item.add_dst(i);
      added = true;
    }

    for (auto r : out.use) {
      assert(r < (int) size());
      auto &item = get(r);
      item.add_src(i);
      added = true;
    }

#if 0
    if (do_accumulators && added) {
      warn << "RegUsage::set_used instr has acc's; "
           << i << ": " << instr.mnemonic(false);
    }
#endif    
  }
}


void RegUsage::set_live(Liveness &live) {
  for (int i = 0; i < live.size(); i++) {
    auto &item = live[i];  // item holds list of accumulator indexes.

    for (auto it : item) {
      auto &item2 = (*this)[it];
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


std::string RegUsage::dump() const {
  if (empty()) return "<Empty>";

  std::string ret;

  for (int i = 0; i < (int) size(); i++) {
    auto const &item = (*this)[i];
    ret << i << ": " << item.dump() << "\n";
  }

  std::string buf;
  auto tmp = get_assigned_only_list(*this);
  if (!tmp.empty()) {
    buf << "Only assigned: " << tmp << "\n";
  }

  tmp = get_never_assigned_list(*this);
  if (!tmp.empty()) {
    buf << "Never assigned: " << tmp << "\n";
  }

  if (!buf.empty()) {
    ret << "\n" << buf;
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

    if (cur.use_overlaps(item)) {
      //info << "check_overlap_usage: Detected conflicting usage of replacement acc";
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
 * @return index of (lowest) available accumulator if found, -1 otherwise.
 */
int RegUsage::dst_range(int line_number) const {
  //info << "Called dst_range line_number: " << line_number;
  assert(!Platform::compiling_for_vc7());

  int first_unused_acc = -1;

  for (int i = 0; i < (int) size(); ++i) {
    auto const &item = get(i);
    if (item.unused()) {
      first_unused_acc = i;  // Found available acc
      break;
    }

    auto &use_dst = item.use_dst();
    if (use_dst.empty()) continue;  // TODO move to in_use

    bool available = !item.in_use(line_number);

    if (available) {
      // Found available accumulator
      first_unused_acc = i;
      break;
    }
  }

  //
  // Incredibly, following exclusion works for vc6. This might fail in the future
  //
  if (Platform::compiling_for_vc4()) {
    if (first_unused_acc == 5) {
      warn << "dst_range blocking special accumulator ACC5.";
      first_unused_acc = -1;
    }

    //
    // Defiant testing indicates that acc4 _can_ actually be used as a general purpose register.
    // Unit tests pass just fine.
    // However, we will respect the vc4 doc (for now TODO).
    //
    if (first_unused_acc == 4) {
      //warn << "dst_range returning special accumulator ACC " << first_unused_acc << "; check for conflicts.";
      warn << "dst_range blocking special accumulator ACC4.";
      first_unused_acc = -1;
    }
  }

  return first_unused_acc;
}

}  // namespace V3DLib
