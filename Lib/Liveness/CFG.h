#ifndef _V3DLIB_LIVENESS_CFG_H_
#define _V3DLIB_LIVENESS_CFG_H_
#include <vector>
#include "Target/instr/Instr.h"
#include "Support/RegIdSet.h"
#include "Range.h"

namespace V3DLib {

typedef int InstrId;                  // Index of instruction in instruction list
using Succs = RegIdSet;               // Set of successors.


/**
 * @brief Control Flow Graph (CFG)
 *
 * Track successor instructions per instruction.
 * A successor instruction precedes the current instruction in the program flow.
 *
 * If there is no successor list defined, only the preceding instruction is a successor.  
 * An extra successor is a jump from a branch to this instruction.
 *
 * The exception to the successor list is the final Target item, which has
 * an explicit empty successor list.
 * This is a label, thus a branch destination and not an instruction.
 *
 * The code is arranged into blocks; a block starts with an instruction with  multiple successors.
 * Blocks may be completely contained in other blocks; they are not allowed to overlap.
 * dd
 */
class CFG : public std::vector<Succs> {
  using Parent = std::vector<Succs>;

public:
  void build(Target::Instr::List &instrs);
  int  block_at(InstrId line_num) const;
  int  block_end(InstrId line_num) const { return blocks.end(line_num);}
  bool is_parent_block(InstrId line_num, int block) const;
  void clear();

  std::string dump() const;

private:

  /**
   * Blocks are numbered uniquely and consecutively as encountered.
   *
   * An child (embedded) block always has a higher number than its parent block.
   * However, a consecutive block can also be numbered lower.
   */
  struct Blocks {
    void build(CFG const &cfg);
    void clear();
    std::string dump() const;
    int end(InstrId line_num) const;
    bool block_in(int cur_block, int parent_block) const;

    std::vector<int>   list;
    std::vector<Range> ranges;

  private:
    void add_ranges();
    int max_block_num() const;
  } blocks;

  bool is_regular(InstrId i) const;
};


}  // namespace V3DLib

#endif  // _V3DLIB_LIVENESS_CFG_H_
