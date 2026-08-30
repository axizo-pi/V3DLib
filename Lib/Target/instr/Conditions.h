#ifndef _V3DLIB_TARGET_SYNTAX_INSTR_CONDITIONS_H_
#define _V3DLIB_TARGET_SYNTAX_INSTR_CONDITIONS_H_
#include "Source/Op.h"     // BaseType
#include <string>
#include <cstdint>

namespace V3DLib {

class CmpOp;  // Forward declaration

// ============================================================================
// Conditions
// ============================================================================

enum Flag {
    ZS              // Zero set
  , ZC              // Zero clear
  , NS              // Negative set
  , NC              // Negative clear
};


///////////////////////////////////////////////////////////////////////////////
// Class BranchCond
///////////////////////////////////////////////////////////////////////////////


struct BranchCond {
  enum Tag {
    COND_ALL,         // Reduce vector of bits to a single
    COND_ANY,         // bit using AND/OR reduction
    COND_ALWAYS,
    COND_NEVER
  };

  Tag tag;            // ALL or ANY reduction?
  Flag flag;          // Condition flag

  BranchCond negate() const;
  bool is_always() const { return tag == COND_ALWAYS; }

  uint8_t encode() const;
  std::string dump() const;
};


///////////////////////////////////////////////////////////////////////////////
// Class SetCond
///////////////////////////////////////////////////////////////////////////////

// v3d only
struct SetCond {
  enum Tag {
    NO_COND,
    Z,
    N,
    C
  };

  bool flags_set() const { return m_tag != NO_COND; }
  void tag(Tag tag) { m_tag = tag; }
  Tag tag() const { return m_tag; }
  void clear() { tag(NO_COND); }
  std::string dump() const;
  void setFlag(Flag flag);

private:
  Tag m_tag = NO_COND;

  const char *to_string() const;
};


///////////////////////////////////////////////////////////////////////////////
// Class AssignCond
///////////////////////////////////////////////////////////////////////////////

/**
 * Assignment conditions
 */
struct AssignCond {
  enum Tag {
    NEVER,
    ALWAYS,
    FLAG
  };

  AssignCond() = default;
  AssignCond(CmpOp const &cmp_op);
  AssignCond(Tag in_tag, Flag in_flag = ZC);

  Tag  tag()  const { return m_tag; }
  Flag flag() const { return m_flag; }
  bool is_always() const { return m_tag  == ALWAYS; }
  bool is_never()  const { return m_tag  == NEVER;  }
  bool is_float()  const { return m_type == FLOAT;  }
  AssignCond negate() const;

  bool operator==(AssignCond rhs) const;
  bool operator!=(AssignCond rhs) const { return !(*this == rhs); }

  uint8_t encode() const;
  std::string dump() const;
  BranchCond to_branch_cond(bool do_all) const;

private:
  Tag      m_tag  = ALWAYS; // Kind of assignment condition
  Flag     m_flag = ZC;     // Condition flag
  BaseType m_type = INT32;
};

extern AssignCond always;   // Is a global to reduce eyestrain in gdb
extern AssignCond never;    // idem

}  // namespace V3DLib

#endif  // define _V3DLIB_TARGET_SYNTAX_INSTR_CONDITIONS_H_
