#ifndef _V3DLIB_SOURCE_STMTSTACK_H_
#define _V3DLIB_SOURCE_STMTSTACK_H_
#include <functional>
#include <vector>
#include <map>
#include "Common/Stack.h"
#include "Stmt.h"

namespace V3DLib {


/**
 * Stack of statements holding conversions
 * during compilation of the  user-language language.
 *
 * Each element is a sequence of statements. Statements are
 * instances of class `Stmt`.
 *
 * `StmtStack` is used to retain the partial conversions
 * of the user-level language to the Source-level language.
 *
 * The top-level item is the current item handled. When complete,
 * it is merged into the previous item.
 *
 * At the end of a kernel compilation, there should be
 * exactly one item on the stack. This is the entire compiled
 * program.
 *
 * ==============
 * Notes
 * -----
 *
 * Internally, `StmtStack` uses a global hidden pointer to the 'current'
 * stack. This is currently required by the programming logic and allows
 * for multiple kernels to be compiled within a single program.
 * However this means that **kernel compilation is not thread-safe**.
 */
class StmtStack : public Stack<Stmts> {
  using Parent = Stack<Stmts>;

public:
  using Ptr = std::shared_ptr<StmtStack>;

  void push();
  void push(Stmt::Ptr s);
  Stmt::Ptr last_stmt(bool do_assert = true);

  void reset();
  void append(Stmt::Ptr stmt);
  void append(Stmts const &stmts);

  StmtStack &operator<<(Stmt::Ptr stmt)      { append(stmt);  return *this; }
  StmtStack &operator<<(Stmts const &stmts)  { append(stmts); return *this; }

  std::string dump() const;

  void merge_top_block();

  //
  // Prefetch Support - If I get the chance to remove prefetches, I will grab that chance
  //
  void first_prefetch(int prefetch_label);

  void add_prefetch(Pointer &exp, int prefetch_label);
  void add_prefetch(PointerExpr const &exp, int prefetch_label);
  void resolve_prefetches();

private:
  class PrefetchContext {
  public:
    void resolve_prefetches();
    void add_prefetch_label(Stmt::Ptr pre);
    bool tags_empty() const { return m_prefetch_tags.empty(); }
    void post_prefetch(Ptr assign);

  private:
    int prefetch_count = 0;

    Stmts              m_prefetch_tags;
    std::vector<Ptr>   m_assigns;

    bool empty() const { return m_prefetch_tags.empty() &&  m_assigns.empty(); }
  };

  std::map<int, PrefetchContext> prefetches;

  void add_prefetch_label(int prefetch_label);
};


StmtStack &stmtStack();
void clearStack();
void initStack(StmtStack &stmtStack);


inline void prefetch(int prefetch_label = 0) {
  stmtStack().first_prefetch(prefetch_label);
}


template<typename T>
void prefetch(T &dst, PointerExpr src, int prefetch_label = 0) {
  stmtStack().first_prefetch(prefetch_label);
  receive(dst);
  stmtStack().add_prefetch(src, prefetch_label);
}


template<typename T>
void prefetch(T &dst, Pointer &src, int prefetch_label = 0) {
  stmtStack().first_prefetch(prefetch_label);
  receive(dst);
  stmtStack().add_prefetch(src, prefetch_label);
}


int prefetch_label();


using StackCallback = std::function<void()>;
Stmts tempStmt(StackCallback f);

}  // namespace V3DLib

#endif  // _V3DLIB_SOURCE_STMTSTACK_H_
