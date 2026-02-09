#include "StmtStack.h"
#include <iostream>          // std::cout
#include "Support/basics.h"
#include "Source/gather.h"
#include "global/log.h"

using namespace Log;

namespace V3DLib {
namespace {

StmtStack *p_stmtStack = nullptr;  // This is not an malloc'd instance but an external reference

StmtStack::Ptr tempStack(StackCallback f) {
  StmtStack::Ptr stack;
  stack.reset(new StmtStack);
  stack->reset();

  // Temporarily replace global stack
  StmtStack *global_stack = p_stmtStack;
  p_stmtStack = stack.get();

  f();  

  p_stmtStack = global_stack;
  return stack;
}

} // anon namespace


Stmts tempStmt(StackCallback f) {
  StmtStack::Ptr assign = tempStack(f);
  assert(assign->size() == 1);
  return *assign->top();
}


///////////////////////////////////////////////////////////////////////////////
// Class StmtStack::PrefetchContext
///////////////////////////////////////////////////////////////////////////////

void StmtStack::PrefetchContext::resolve_prefetches() {
  if (m_prefetch_tags.empty()) {
    return;  // nothing to do
  }
  assert(m_prefetch_tags.size() == m_assigns.size() + 1);  // One extra for the initial prefetch tag

  // first prefetches go to first tag
  for (int i = 0; i < Platform::gather_limit() &&  i < (int) m_assigns.size(); ++i) {
    auto assign = m_assigns[i];

    assert(assign->size() == 1);
    auto &assigns = *assign->top();

    assert(!m_prefetch_tags.empty());
    m_prefetch_tags[0]->append(assigns);
  }

  for (int i = 1; i < (int) m_prefetch_tags.size(); ++i) {
    int assign_index = i + (Platform::gather_limit() - 1);

    if (assign_index >= (int) m_assigns.size()) {
      break;
    }

    assert(!m_prefetch_tags.empty());
    assert(m_assigns[assign_index]->size() == 1);

    auto &assigns = *m_assigns[assign_index]->top();
    m_prefetch_tags[i]->append(assigns);
  }

  m_prefetch_tags.clear();
  m_assigns.clear();

  assertq(empty(), "Still prefetch assigns present after compile");
}


void StmtStack::PrefetchContext::post_prefetch(Ptr assign) {
  assert(assign.get() != nullptr);
  assert(assign->size() == 1);  // Not expecting anything else
  assert(prefetch_count <= Platform::gather_limit());
  assert(!m_prefetch_tags.empty());

  m_assigns.push_back(assign);
}


void StmtStack::PrefetchContext::add_prefetch_label(Stmt::Ptr pre) {
  m_prefetch_tags.push_back(pre);
}


///////////////////////////////////////////////////////////////////////////////
// Class StmtStack
///////////////////////////////////////////////////////////////////////////////

void StmtStack::add_prefetch_label(int prefetch_label) {
  auto pre = Stmt::create(Stmt::GATHER_PREFETCH);
  append(pre);
  prefetches[prefetch_label].add_prefetch_label(pre);
}


void StmtStack::first_prefetch(int prefetch_label) {
  if (!prefetches[prefetch_label].tags_empty()) return;
  add_prefetch_label(prefetch_label);
}


/*  // Intentionally not doxygen
 * Technically, the param should be a pointer in some form.
 * This is not enforced right now.  
 * **TODO** find a way to enforce this
 *
 * @return true if param added to prefetch list,
 *         false otherwise (prefetch list is full)
 */
void StmtStack::add_prefetch(PointerExpr const &exp, int prefetch_label) {
  add_prefetch_label(prefetch_label);

  Ptr assign = tempStack([&exp] {
    Pointer ptr = exp;
    gatherBaseExpr(exp);
  });

  prefetches[prefetch_label].post_prefetch(assign);
}


void StmtStack::add_prefetch(Pointer &exp, int prefetch_label) {
  add_prefetch_label(prefetch_label);

  Ptr assign = tempStack([&exp] {
    gatherBaseExpr(exp);
    exp.inc();
  });

  prefetches[prefetch_label].post_prefetch(assign);
}


void StmtStack::resolve_prefetches() {
  for (auto &item : prefetches) {
    item.second.resolve_prefetches();
  }
}


void StmtStack::push(Stmt::Ptr s) {
  if (empty()) {
    push();
  }

  top()->push_back(s);
}


/**
 * Return the last added statement on the stack.
 *
 * This **does not mean** the current item at the top to the stack,
 * because the stack items are **sequences of statements**.
 * The last added statement will be the last statement in the top item of the stack.
 *
 * Param `do_assert` is added to retain original working, while giving flexibility
 * to handle null pointers for absent items.
 *
 * @param do_assert  if true, assert if last statement not found.
 * @return           Pointer to the last added statement,
 *                   `nullptr` if not present and assertions not enabled.
 */
Stmt::Ptr StmtStack::last_stmt(bool do_assert) {
	if (empty()) {
		assertq(!do_assert, "last_stmt() stack is empty");
		return nullptr;
	}

  auto level = Parent::top();  // Should be non-null due to check empty(); we check anyway
  if (level == nullptr || level->empty()) {
		assertq(!do_assert, "last_stmt() top statement is empty");
		return nullptr;
	}

  auto ptr = level->back();    // Should be non-null due to previous test; we check anyway
  if (ptr.get() == nullptr) {
		assertq(!do_assert, "last_stmt() first back statement is empty");
		return nullptr;
	}

  return ptr;
}


/**
 * Create a new level in the stack
 */
void StmtStack::push() {
  Stack::Ptr stmts(new Stmts());
  Parent::push(stmts);
}


void StmtStack::reset() {
  clear();
  push();

  prefetches.clear();
}


/**
 * Add passed statement to the end of the current instructions
 */
void StmtStack::append(Stmt::Ptr stmt) {
  assert(stmt.get() != nullptr);

  if (empty()) push(); // add empty item on empty stack
  Parent::top()->push_back(stmt);
}


void StmtStack::append(Stmts const &stmts) {
  for (int i = 0; i < (int) stmts.size(); i++) {
    append(stmts[i]);
  }
}


std::string StmtStack::dump() const {
  using ::operator<<;  // C++ weirdness

  std::string ret;

  each([&ret] (Stmts const &item) {
    for (int i = 0; i < (int) item.size(); i++) {
      ret << item[i]->dump(true) << "\n";
    }
  });

  return ret;
}


/**
 * Added top-level item in the stack to the next item as a block.
 *
 * The next item must be a statement requiring blocks
 * (`IF`, `WHERE`, `WHILE`, `FOR`).
 *
 * This is called from function `End_()`, in file `Lang.cpp`.
 * There should be no other place where this method is called.
 *
 * **Assumption**: top element is a complete block.
 */
void StmtStack::merge_top_block() {
  auto block_ptr = top();  // Remove top-level item
  pop();

  // Merge into next item in stack
  Stmt::Ptr s = last_stmt();
  if (!s->add_block(*block_ptr)) {
    cerr << "Syntax error: unexpected block" << thrw;
  }

  // TODO: I think following is not necessary, the top stack item is changed while still on the stack.
  //       Examine this
  pop();
  append(s);
}


StmtStack &stmtStack() {
  assert(p_stmtStack != nullptr);
  return *p_stmtStack;
}


void clearStack() {
  if (p_stmtStack == nullptr) {  // May occur if error during initialization
    return;
  }

  p_stmtStack->resolve_prefetches();
  p_stmtStack = nullptr;
}


void initStack(StmtStack &stmtStack) {
  if (p_stmtStack != nullptr) {
    //
    // This can happen if a compilation of a preceding kernel fails
    //
    // Another option is that tempStack() was called, which means that an malloc'd intance
    // is pointed to by p_stmtStack.
    // We ignore this option, it occurs under very specific circumstances
    //

    Log::cerr << "initStack(): p_stmtStack set on entry. Ignoring.";
  }

  stmtStack.reset();
  p_stmtStack = &stmtStack;
}


/**
 * Generate a new prefetch label
 */
int prefetch_label() {
  static int count = 0;
  return ++count;
}

}  // namespace V3DLib
