#include "Lang.h"
#include <stdio.h>
//#include "Support/basics.h"  // fatal()
#include "Source/Int.h"
#include "StmtStack.h"
#include "global/log.h"
#include "vc4/Functions.h"

namespace V3DLib {
namespace {

void prepare_stack(Stmt::Ptr s) {
  auto &stack = stmtStack();
  stack.push();
  stack.push(s);
  stack.push();
}

}  // anon namespace

//=============================================================================
// Assignment token
//=============================================================================

void assign(Expr::Ptr lhs, Expr::Ptr rhs) {
  stmtStack() << Stmt::create_assign(lhs, rhs);
}


//=============================================================================
// 'Else' token
//=============================================================================

void Else_() {
  auto block_ptr  = stmtStack().top();
  stmtStack().pop();

  Stmt::Ptr s = stmtStack().last_stmt();

  if (!s->then_block(*block_ptr)) {
    error("Syntax error: 'Else' without preceding 'If' or 'Where'");
  }

  stmtStack().push();  // Set top stack item for else-block
}


//=============================================================================
// 'End' token
//=============================================================================

void End_() {
	stmtStack().merge_top_block();
}


//=============================================================================
// Tokens with block handling
//=============================================================================

void If_(Cond c) {
  Stmt::Ptr s = Stmt::create(Stmt::IF);
  s->cond(c.cexpr());
  prepare_stack(s);
}

void If_(BoolExpr b) { If_(any(b)); }


void While_(Cond c) {
  Stmt::Ptr s = Stmt::create(Stmt::WHILE);
  s->cond(c.cexpr());
  prepare_stack(s);
}

void While_(BoolExpr c) { While_(any(c)); }


void For_(Cond c) {
  Stmt::Ptr s = Stmt::create(Stmt::FOR);
  s->cond(c.cexpr());
  prepare_stack(s);
}


void For_(BoolExpr b)   { For_(any(b)); }


void Where__(BExpr::Ptr b) {
  Stmt::Ptr s = Stmt::create(Stmt::WHERE);
  s->where_cond(b);
  prepare_stack(s);
}


//=============================================================================
// 'For' handling
//=============================================================================

void ForBody_() {
  auto inc = stmtStack().top();
  assert(inc);
  stmtStack().pop();

  Stmt::Ptr s = stmtStack().last_stmt();
  s->inc(*inc);

  stmtStack().push();
}


/**
 * Has no inputs, only an output, which is always magic reg SYNCB.
 *
 * `barrier` is v3d-specific. vc4 will need a different implementation,
 * most likely with semaphores.
 */
void barrier() {
	if (Platform::compiling_for_vc4()) {
		Log::warn << "barrier compiling for vc4";
		// vc4 - Stmt::BARRIER will not be passed on
		vc4::barrier();
	} else {
		// v3d
		stmtStack().push(Stmt::create(Stmt::BARRIER));
	}
}


//=============================================================================
// Comments and breakpoints
//=============================================================================

void header(char const *str) {
  stmtStack().last_stmt()->header(str);
}


void comment(char const *str) {
  stmtStack().last_stmt()->comment(str);
}


void break_point(bool val) {
  if (val) {
    stmtStack().last_stmt()->break_point();
  }
}

}  // namespace V3DLib
