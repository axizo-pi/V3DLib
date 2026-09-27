#ifndef _V3DLIB_LIVENESS_REGUSAGE_H_
#define _V3DLIB_LIVENESS_REGUSAGE_H_
#include <vector>
#include <string>
#include "Target/instr/Instr.h"
#include "Range.h"

namespace V3DLib {

struct RegUsageItem {
  Reg reg;

  void add_dst(int n, bool is_cond_assign);
  void add_src(int n);
  void add_live(int n);
  bool unused() const;
  bool only_assigned() const  { return !m_use_dst.empty() && m_src_range.count() == 0; }
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

  bool regular_use() const {
    return !(unused() || only_assigned());
  }

  void reset();
  bool empty() const;
  std::vector<int>  const &use_dst() const { return m_use_dst; }
  Range const &src_range() const { return m_src_range; }

private:
  Range m_src_range;           // First and last instructions where var is used as src
  std::vector<int> m_use_dst;  // List of line numbers where var is set
  Range m_live_range;

  bool valid(bool disp) const;
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
  std::string dump(bool verbose = false) const;
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
