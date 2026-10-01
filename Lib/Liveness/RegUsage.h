#ifndef _V3DLIB_LIVENESS_REGUSAGE_H_
#define _V3DLIB_LIVENESS_REGUSAGE_H_
#include <vector>
#include <string>
#include "Target/instr/Instr.h"
#include "Range.h"

namespace V3DLib {

/**
 * =============================================
 * Notes
 * -----
 *
 * 1. Special case: multiple dst's, no live range.
 *
 *    The register is only written to.
 *    Better would be to write to reg NOP here; however at time of writing
 *    it still occurs and the spec's allow it, so this case must be taken into account.
 *
 *    So we lie a bit and handle only the final write.
 *    Occurances _will_ be logged, however. See `introduceAccum()`.
 *
 *    **NOTE:** This case occurs often on `vc6` for the initial handling of QPU Id and QPU Num.
 *              This is actually benevolent and fixed later with optimization and acc replacement.
 *              Will not be flagged as a special case.
 *
 * 2. Live range is **not** used for accumulator liveness.
 *
 * 3. Special assertion cases which have been disproved:
 *   - Following is true most of the time, but _not_ always:
 *
 *       (m_use_dst[0] + 1 == m_live_range.first())
 *
 *     Register writes need not be followed by a read.
 *
 *   - Following is also not always true:
 *
 *       (m_src_range.last() == m_live_range.last());
 *
 *     In blocks and loops, the liveness range can be extended to well beyond the last assignment.
 */
struct RegUsageItem {
  Reg reg;

  void add_dst(int n, bool is_cond_assign);
  void add_src(int n);
  void add_live(int n);
  bool unused() const;
  bool only_assigned() const;
  bool never_assigned() const { return !unused() && m_use_dst.empty(); }
  bool assigned_once() const;
  std::string dump() const;
  int live_range() const;
  int use_range() const;
  int first_dst() const;
  int first_live() const      { return m_live_range.first(); }
  int last_live() const       { return m_live_range.last(); }
  int first_usage() const;
  int last_usage() const;
  bool use_overlaps(RegUsageItem const &rhs) const;
  bool regular_use() const { return !(unused() || only_assigned()); }

  void reset();
  bool empty() const;
  std::vector<int>  const &use_dst() const { return m_use_dst; }
  Range const &src_range() const { return m_src_range; }

private:
  Range m_src_range;           // First and last instructions where var is used as src
  std::vector<int> m_use_dst;  // List of line numbers where var is set
  Range m_live_range;

  bool check_valid(bool do_throw = true) const;
};


class Liveness;

struct RegUsage : private std::vector<RegUsageItem> {
  using Parent = std::vector<RegUsageItem>;
  using Parent::size;
  using Parent::operator[];

  RegUsage(int numVars);

  void reset();
  void set_used(Target::Instr::List const &instrs, bool do_accumulators);
  void set_live(Liveness &live);
  std::string dump(bool verbose) const;
  std::string dump() const { return dump(true); }
  void check() const;
  std::string dump_use_ranges() const;
  bool check_overlap_usage(Reg acc, RegUsageItem const &item) const;

  int dst_range(int line_number) const;

private:
  RegUsageItem &get(int i);
  RegUsageItem const &get(int i) const;
  std::string allocated_registers_dump() const;
};

}  // namespace V3DLib

#endif  // _V3DLIB_LIVENESS_REGUSAGE_H_
