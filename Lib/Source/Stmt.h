#ifndef _V3DLIB_SOURCE_STMT_H_
#define _V3DLIB_SOURCE_STMT_H_
#include "Support/InstructionComment.h"
#include "Int.h"
#include "Expr.h"
#include "CExpr.h"
#include "vc4/DMA/DMA.h"

namespace V3DLib {

// ============================================================================
// Class Stmt
// ============================================================================

/**
 * Representation of source-level instructions, directly translated from
 * the user-level language.
 *
 * A `Stmt` instance is either a single statement (eg. `ASSIGN`) or arrays
 * of statements for the top-level block instructions (eg. `FOR`).
 *
 * ## Statement Blocks
 *
 * The main member elements are:
 *
 * - **Array m_stmts_a**: Array of statements in default block; in the case
 *                        of conditional statements, this is the THEN-block.
 * - **Array m_stmts_b**: For conditional statements with ELSE-parts,
 *                        (`IF`, `WHERE`), contains the array of statements in
 *                        the ELSE-part.
 *
 * =====
 * Notes
 * -----
 *
 * The original implementation put instances of `Stmt` on a custom heap.
 * This placed a strict limit on compilation size, which I continually ran
 * into, and complicated initialization of instances (notably, ctors were
 * not called).  
 * The custom heap has thus been removed and instances are allocated in
 * the regular C++ way.  
 *
 * Pointers within this definition are in the process of being replaced
 * with smart pointers.  
 * **TODO** check if done
 */
struct Stmt : public InstructionComment {
  using Ptr = std::shared_ptr<Stmt>;

  enum Tag {
    SKIP,
    NOP,
    ASSIGN,
    SEQ,
    WHERE,
    IF,
    WHILE,
    FOR,
    LOAD_RECEIVE,

    GATHER_PREFETCH,

    // DMA stuff
    SET_READ_STRIDE,
    SET_WRITE_STRIDE,
    SEND_IRQ_TO_HOST,
    SEMA_INC,
    SEMA_DEC,
    SETUP_VPM_READ,
    SETUP_VPM_WRITE,
    SETUP_DMA_READ,
    SETUP_DMA_WRITE,
    DMA_READ_WAIT,
    DMA_WRITE_WAIT,
    DMA_START_READ,
    DMA_START_WRITE,

    BARRIER,

    NUM_TAGS
  };

  class Array : public std::vector<Ptr> {
  public:
    std::string dump(bool show_comments = false) const;
  };

  Stmt(Tag in_tag) : tag(in_tag) {}

  ~Stmt();

  Stmt &header(std::string const &msg) { InstructionComment::header(msg);  return *this; }
  Stmt &comment(std::string msg)       { InstructionComment::comment(msg); return *this; }

  std::string disp() const { return disp_intern(false, 0); }
  std::string dump(bool show_comments = false) const { return disp_intern(true, 0, show_comments); }
  void append(Array const &rhs);

  //
  // Accessors for pointer objects.
  // They basically all do the same thing, but in a controlled manner.
  // Done like this to ease the transition to smart pointers.
  // Eventually this can be cleaned up.
  //
  BExpr::Ptr where_cond() const;
  void where_cond(BExpr::Ptr cond);

  Expr::Ptr lhs() const;
  void lhs(Expr::Ptr val);
  Expr::Ptr rhs() const;
  void rhs(Expr::Ptr val);
  Expr::Ptr assign_lhs() const;
  Expr::Ptr assign_rhs() const;
  Expr::Ptr address();

  Array const &then_block() const;
  bool then_block_empty() const;
  bool then_block(Array const &in_block);
  Array const &else_block() const;
  bool else_block_empty() const;
  bool add_block(Array const &in_block);
  Array const &body() const;

  void inc(Array const &arr);

  void cond(CExpr::Ptr cond);
  CExpr::Ptr if_cond() const;
  CExpr::Ptr loop_cond() const;

  //
  // Instantiation methods
  //
  static Ptr create(Tag in_tag);
  static Ptr create(Tag in_tag, Expr::Ptr e0, Expr::Ptr e1);
  static Ptr create_assign(Expr::Ptr lhs, Expr::Ptr rhs);

  Tag tag;
  DMA::Stmt dma;

private:
  BExpr::Ptr m_where_cond;

  Expr::Ptr m_exp_a;
  Expr::Ptr m_exp_b;

  Array m_stmts_a;
  Array m_stmts_b;

  CExpr::Ptr m_cond;

  static Ptr create(Tag in_tag, Ptr s0, Ptr s1);
  std::string disp_intern(bool with_linebreaks, int seq_depth = 0, bool show_comments = false) const;
  bool check_blocks() const;
};


class Stmts : public Stmt::Array {
public:
  std::string dump() const;
};

}  // namespace V3DLib

#endif  // _V3DLIB_SOURCE_STMT_H_
