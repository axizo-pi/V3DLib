#include "Translate.h"
#include "Support/Platform.h"
#include "Support/Helpers.h"   // contains()
#include "SourceTranslate.h"
#include "Target/SmallLiteral.h"
#include "Target/instr/Mnemonics.h"
#include "Support/basics.h"

using namespace Log;

namespace V3DLib {

using ::operator<<;  // C++ weirdness

namespace {

// ============================================================================
// Operands
// ============================================================================

/**
 * Translate the argument of an operator (either a variable or a small imm)
 */
RegOrImm operand(Expr::Ptr e) {
  RegOrImm x;

  if (e->tag() == Expr::VAR) {
    x = RegOrImm(e->var());
  } else if (e->tag() == Expr::INT_LIT) { 
    x = Imm(e->intLit);
  } else if (e->tag() == Expr::FLOAT_LIT) {
    x = Imm(e->floatLit);
  } else {
    assert(false);
  }

  return x;
}


// ============================================================================
// Variable assignments
// ============================================================================

/**
 * Translate an expression to a simple expression, generating
 * instructions along the way.
 */
Expr::Ptr simplify(Instr::List *seq, Expr::Ptr e) {
  if (e->isSimple()) {
    return e;
  }

  Var tmp_var = VarGen::fresh();

  Instr::List tmp;
  tmp << varAssign(tmp_var, e);
  //tmp.front().comment("simplify varAssign");
  *seq << tmp;

  return mkVar(tmp_var);
}


// ============================================================================
// Assignment statements
// ============================================================================

/**
 * @brief Encode assign statements to Target code
 *
 * @param target   Target instruction sequence to extend
 * @param lhs      Expression on left-hand side
 * @param rhs      Expression on right-hand side
 */
void assign(Instr::List &target, Expr::Ptr lhs, Expr::Ptr rhs) {
  //Log::warn << "assign lhs: '" << lhs->dump() << "', rhs: " << rhs->dump();

  // -----------------------------------------------------------
  // Case: v := rhs, where v is a variable and rhs an expression
  // -----------------------------------------------------------
  if (lhs->tag() == Expr::VAR) {
    auto tmp = varAssign(lhs->var(), rhs);
    //Log::warn << "VAR target: " << tmp.dump();
    target << tmp;
    return;
  }

  // ---------------------------------------------------------
  // Case: *lhs := rhs where lhs is not a var or rhs not a var
  // ---------------------------------------------------------
  if (lhs->tag() == Expr::DEREF && (lhs->deref_ptr()->tag() != Expr::VAR || rhs->tag() != Expr::VAR)) {
    assert(!lhs->deref_ptr()->isLit());
    lhs->deref_ptr(simplify(&target, lhs->deref_ptr()));
    rhs = putInVar(&target, rhs);
  }

  // -------------------------------------------------
  // Case: *v := rhs where v is a var and rhs is a var
  // -------------------------------------------------

  // Strictly speaking, the two VAR tests are not necessary; it is the only possible case
  // (According to previous code, that is)
  if (lhs->tag() == Expr::DEREF
  && (lhs->deref_ptr()->tag() == Expr::VAR || rhs->tag() == Expr::VAR)) {
    target << getSourceTranslate().store_var(lhs->deref_ptr()->var(), rhs->var());
    return;
  }

  assert(false);  // Should not be reachable
}


// ============================================================================
// Condition flag helpers
// ============================================================================

// Each QPU contains an implicit condition vector which can answer
// various questions about each element of a vector:
//
//   * ZS - is zero?
//   * ZC - is non-zero?
//   * NS - is negative?
//   * NC - is non-negative?
//
// The condition vector is modified when the 'setFlags' field of
// an ALU instruction is 'true'.  The condition vector can be read
// from an assignment condition or in a branch condition.

// Evaluating a vector boolean expression results in a condition
// pair <condVar,condFlag> where:
//
//  * condVar is a variable containing a vector of values
//  * condFlag is a condition flag in set {ZS,ZC,NS,NC} (see above)
//
// If 'condVar' is assigned to a register and the 'setFlags' field of
// the assignment is 'true', the implicit condition vector is updated.
// The implicit condition vector can then be queried using 'condFlag'
// to determine the truth of elements in the boolean vector.
//
// For example, assuming vectors of size 4 for simplicity, the result
// of evaluating
//
//   [1,2,3,4] <= [4,1,3,6]
//
// might be the condition pair <[-3,1,0,-2], NC>.
//
// Given two condition pairs <condVarA, condFlagA> and <condVarB,
// condFlagB> we would like to compute the logical disjunction.
//
// Pre-condition: we are in a state where the implicit condition
// vector has been set using the value of condVarB, hence we don't
// need the value of condVarB as an argument.
//
// The value of condVarA will be overwritten with the 'condVar' of the
// disjunction, and the corresponding condFlag will be returned as a
// result.


// ============================================================================
// Boolean expressions
// ============================================================================

/**
 * Creates the instructions for this comparison.
 *
 * The comparison is internally implemented as a subtract-operation.
 */
void cmpExp(Instr::List *seq, BExpr::Ptr bexpr, Var v) {
  BExpr b = *bexpr;
  assert(b.tag() == CMP);

  auto cmp_swap_leftright = [] (BExpr &b) {
    auto tmp   = b.cmp_lhs();
    b.cmp_lhs(b.cmp_rhs());
    b.cmp_rhs(tmp);
  };


  if (b.cmp.op() == CmpOp::GT) {                       // 'x > y', replace with y < x
    cmp_swap_leftright(b);
    b.cmp.op(CmpOp::LT);
  }

  if (b.cmp.op() == CmpOp::LE) {                       // 'x <= y', replace with y >= x
    cmp_swap_leftright(b);
    b.cmp.op(CmpOp::GE);
  }

  if (!b.cmp_lhs()->isSimple()) {                      // 'x op y', where x is not simple
    b.cmp_lhs(simplify(seq, b.cmp_lhs()));
  }

  if (!b.cmp_rhs()->isSimple()) {                      // 'x op y', where y is not simple
    b.cmp_rhs(simplify(seq, b.cmp_rhs()));
  }

  if (b.cmp_rhs()->isLit() && b.cmp_rhs()->isLit()) {  // 'x op y', where x and y are both literals
    Var tmpVar = VarGen::fresh();
    *seq << varAssign(tmpVar, b.cmp_lhs());
    b.cmp_lhs(mkVar(tmpVar));
  }


  //
  // At this point x and y are simple
  //
  using namespace V3DLib::Target::instr;

  Var dummy  = VarGen::fresh();
  Var dummy2 = VarGen::fresh();
  AssignCond assign_cond(b.cmp);

  // Implement comparison using subtraction instruction
  Op op(SUB, b.cmp.type());

  Instr instr(ALU);
  instr.setCondOp(b.cmp);
  instr.ALU.op       = ALUOp(op);
  instr.ALU.srcA     = operand(b.cmp_lhs());
  instr.ALU.srcB     = operand(b.cmp_rhs());
  instr.dest(dummy);

  auto mov1 = mov(v, 1);
  assert(mov1.size() == 1);
  mov1.back().cond(assign_cond);        // TODO: would be better if this used acc-reg

  auto mov2 = mov(dummy2, v);
  assert(mov2.size() == 1);
  mov2.back().setCondFlag(Flag::ZC);  // Reset flags so that Z-flag is used

  *seq << li(v, 0).comment("Store condition as Bool var")
       << instr
       << mov1
       << mov2
  ;

  seq->back().comment("End store condition as Bool var");
}


AssignCond boolExp(Instr::List *seq, BExpr::Ptr bexpr, Var v);  // Forward declaration


void boolVarExp(Instr::List &seq, BExpr b, Var v) {
  using namespace V3DLib::Target::instr;

  // TODO maybe not necessary, check. Otherwise, use v directly
  Var v1 = VarGen::fresh();
  boolExp(&seq, b.lhs(), v1);  // return val ignored

  Var w = VarGen::fresh();
  boolExp(&seq, b.rhs(), w);  // idem

  if (b.tag() == OR) {
    seq << bor(v, v1, w).setCondFlag(Flag::ZC).comment("Bool var OR");
  } else if (b.tag() == AND) {
    seq << band(v, v1, w).setCondFlag(Flag::ZC).comment("Bool var AND");
  } else {
    assert(false);
  }
}


/**
 * Handle general boolean expressions.
 *
 * Boolean conditions var's are used, to unify the differing approaches
 * to flag checking in `v3d` and `vc4`.
 *
 * The condition result is stored as booleans (with 0/1) in `v`, indicating the truth
 * value of the flag tests.
 *
 * The condition is reset to always use Z. Checks should be on ZC (zero clear) for 1 == true.
 *
 * 
 * @param seq    instruction sequence to which the instructions to evaluate the
 *               expression are appended
 * @param bexpr  the boolean expression to evaluate;
 * @param v      condVar 'v' to which the evaluated expression will be written to
 *
 * @return the condition to use when checking the flags for this comparison
 */
AssignCond boolExp(Instr::List *seq, BExpr::Ptr bexpr, Var v) {
  using namespace V3DLib::Target::instr;
  BExpr b = *bexpr;

  switch (b.tag()) {
    case CMP:
      cmpExp(seq, bexpr, v);
    break;
    case NOT: {          // '!b', where b is a boolean expression
      boolExp(seq, b.neg(), v);
      *seq << bxor(v, v, 1).setCondFlag(Flag::ZC);
    }
    break;
    case OR:             // 'a || b', where a, b are boolean expressions
    case AND:            // 'a && b', where a, b are boolean expressions
      boolVarExp(*seq, b, v);
      break;
    default:
      assert(false);
      break;
  }

  return AssignCond(CmpOp(CmpOp::NEQ, INT32));  // Wonky syntax to get the flags right
}


// ============================================================================
// Conditional expressions
// ============================================================================

BranchCond condExp(Instr::List &seq, CExpr &c) {
  Instr::List ret;
  Var v = VarGen::fresh();
  AssignCond cond = boolExp(&ret, c.bexpr(), v);

  //Log::warn << "condExp seq:\n" << ret.dump();
  seq << ret;

  return cond.to_branch_cond(c.tag() == ALL);
}


// ============================================================================
// Where statements
// ============================================================================

Instr::List whereStmt(Stmt::Ptr s, Var condVar, AssignCond cond, bool saveRestore);

Instr::List whereStmt(
  Stmt::Array const &src,
  Var condVar,
  AssignCond cond,
  bool saveRestore,
  bool first_true = false
) {
  Instr::List ret;

  for (int i = 0; i < (int) src.size(); i++) {
    ret << whereStmt(src[i], condVar, cond, (i == 0 && first_true)?true:saveRestore);
  }

  return ret;
}


Instr::List whereStmt(Stmt::Ptr s, Var condVar, AssignCond cond, bool saveRestore) {
  using namespace V3DLib::Target::instr;
  Instr::List ret;

  if (s.get() == nullptr) return ret;
  if (s->tag == Stmt::SKIP) return ret;

  // ------------------------------------------------------
  // Case: v = e, where v is a variable and e an expression
  // ------------------------------------------------------
  if (s->tag == Stmt::ASSIGN && s->assign_lhs()->tag() == Expr::VAR) {
    assign(ret, s->assign_lhs(), s->assign_rhs());
    ret.back().cond(cond); //.comment("Assign var in Where");
    return ret;
  }

  // ------------------------------------------------------
  // Case: *v = e, where v is a pointer and e an expression
  // ------------------------------------------------------
  if (s->tag == Stmt::ASSIGN && s->assign_lhs()->tag() == Expr::DEREF) {
    assign(ret, s->assign_lhs(), s->assign_rhs());
    ret.back().cond(cond); // .comment("Assign *var (deref) in Where");
    return ret;
  }

  // ---------------------------------------------
  // Case: sequence of statements
  // ---------------------------------------------
  if (s->tag == Stmt::SEQ) {
    breakpoint  // TODO apparently never reached, verify
    ret << whereStmt(s->body(), condVar, cond, saveRestore, true);
    return ret;
  }

  // ----------------------------------------------------------
  // Case: where (b) s0 s1, where b is a boolean expression and
  //                        s0 and s1 are statements.
  // ----------------------------------------------------------
  if (s->tag == Stmt::WHERE) {
    using Target::instr::mov;
    AssignCond andCond(CmpOp(CmpOp::NEQ, INT32));  // Wonky syntax to get the flags right

    Var newCondVar   = VarGen::fresh();
    AssignCond newCond;
    {
      // Compile new boolean expression
      Instr::List seq;
      newCond = boolExp(&seq, s->where_cond(), newCondVar);
      assert(!seq.empty());

      std::string cmt = "Start where (";
      cmt << (cond.is_always()?"always":"nested") << ")";
      seq.front().comment(cmt);

      // This comment is used to signal downstream that this is the
      // statement processing the final step for the where-condition.
      // This statement pushes condition flags for the where-body.
      //
      // It's a dubious thing to use comments to signal this, but currently
      // it's the most obvious thing to use.
      // Used in v3d when combining add/mul alu instructions
      seq.back().comment("where condition final");

      ret << seq;
    }

    if (cond.is_always()) {
      // Top-level handling of where-statements

      // Compile 'then' statement
      if (!s->then_block().empty()) {
        auto seq = whereStmt(s->then_block(), newCondVar, andCond, !s->else_block().empty());
        assert(!seq.empty());
        seq.front().comment("then-branch of where (always)");
        ret << seq;
      }

      // Compile 'else' statement
      if (!s->else_block().empty()) {
        Var v2 = VarGen::fresh();
        ret << bxor(v2, newCondVar, 1).setCondFlag(Flag::ZC);

        auto seq = whereStmt(s->else_block(), v2, andCond, false);
        assert(!seq.empty());
        seq.front().comment("else-branch of where (always)");
        ret << seq;
      }
    } else {
      // Where-statements nested in other where-statements

      if (!s->then_block().empty()) {
        // AND new boolean expression with original condition
        Var dummy   = VarGen::fresh();
        ret << band(dummy, condVar, newCondVar).setCondFlag(Flag::ZC);

        // Compile 'then' statement
        {
          auto seq = whereStmt(s->then_block(), dummy, andCond, false);
          assert(!seq.empty());
          seq.front().comment("then-branch of where (nested)");
          ret << seq;
        }
      }

      if (!s->else_block().empty()) {
        Var v2    = VarGen::fresh();
        Var dummy = VarGen::fresh();

        ret << bxor(v2, newCondVar, 1)
            << band(dummy, condVar, v2).setCondFlag(Flag::ZC);

        // Compile 'else' statement
        {
          auto seq = whereStmt(s->else_block(), dummy, andCond, false);
          assert(!seq.empty());
          seq.front().comment("else-branch of where (nested)");
          ret << seq;
        }
      }
    }

    ret.back().comment("End Where");
    return ret;
  }

  assertq(false, "V3DLib: only assignments and nested 'where' statements can occur in a 'where' statement", true);
  return ret;
}


/**
 * @brief Translate if-then-else statement to target code
 *
 * @param target write-parameter; list of converted target statements
 * @param s      statement to convert
 */
void translateIf(Instr::List &target, Stmt &s) {
  using namespace Target::instr;

  Label endifLabel = freshLabel();
  BranchCond cond  = condExp(target, *s.if_cond());          // Compile condition
    
  if (s.else_block().empty()) {
    target << branch(endifLabel).branch_cond(cond.negate()); // Branch over 'then' statement
    encode_target(target, s.then_block());                   // Compile 'then' statement
  } else {
    Label elseLabel = freshLabel();

    target << branch(elseLabel).branch_cond(cond.negate());  // Branch to 'else' statement

    encode_target(target, s.then_block());                   // Compile 'then' statement

    target << branch(endifLabel)                             // Branch to endif label
           << label(elseLabel);                              // Label for 'else' statement

    encode_target(target, s.else_block());                   // Compile 'else' statement
  }
  
  target << label(endifLabel);                               // Label for endif
}


void translateWhile(Instr::List &seq, Stmt &s) {
  using namespace Target::instr;

  Label startLabel = freshLabel();
  Label endLabel   = freshLabel();
  BranchCond cond  = condExp(seq, *s.loop_cond());     // Compile condition
 
  seq << branch(endLabel).branch_cond(cond.negate())   // Branch over loop body
      << label(startLabel);                            // Start label

  if (!s.body().empty()) {                             // Compile body
    encode_target(seq, s.body());
  }
  condExp(seq, *s.loop_cond());                        // Compute condition again
                                                       // TODO why is this necessary?

  seq << branch(startLabel).branch_cond(cond)          // Branch to start label
      << label(endLabel);                              // End label
}


// ============================================================================
// Statement Handling
// ============================================================================

/**
 * @brief Translate one Source statement to Target statements.
 *
 * The conversion can emit multiple Target-level statements, or none at all.
 *
 * Source-level statements may contain blocks of statements; in that case,
 * this method is called recursively via `encode_target()`.
 *
 * @param s  Source statement to encode.
 * @return   list of encoded Target statements for passed Source statement
 */
Instr::List encode(Stmt::Ptr s) {
  assert(s != nullptr);

  using namespace Target::instr;

  Instr::List ret;

  switch (s->tag) {
    case Stmt::GATHER_PREFETCH:          // Remove if still present
    case Stmt::SKIP:
      return ret;
    case Stmt::ASSIGN:                   // 'lhs = rhs', where lhs and rhs are expressions
      assign(ret, s->assign_lhs(), s->assign_rhs());
      break;
    case Stmt::SEQ:                      // 's0 ; s1', where s1 and s2 are statements
      encode_target(ret, s->body());
      break;
    case Stmt::IF:                       // 'if (c) s0 s1', where c is a condition, and s0, s1 statements
      translateIf(ret, *s);
      break;
    case Stmt::WHILE:                    // 'while (c) s', where c is a condition, and s a statement
      translateWhile(ret, *s);
      break;

    case Stmt::WHERE: {                  // 'where (b) s0 s1', where c is a boolean expr,
                                         // and s0, s1 are statements

      Var condVar = VarGen::fresh();   // This is the top-level definition of condVar
      ret << whereStmt(s, condVar, always, false);
    }
    break;

    case Stmt::LOAD_RECEIVE:             // 'receive(e)', where e is an expr
      assert(s->address()->tag() == Expr::VAR);
      ret << recv(s->address()->var());
      break;

    case Stmt::BARRIER:
      ret << barrier();
      break;

    default:
      if (!getSourceTranslate().stmt(ret, s)) {
        Log::cerr << "stmt() unhandled Stmt tag: " << s->tag << Log::thrw;
      }
      break;
  }

  //assert(!ret.empty());
  if (!ret.empty()) {
    ret.back().transfer_comments(*s);
  } else {
    warn << "Translate encode(), no output for stmt: " << s->dump();
    if (s->has_comments()) {
      warn << "Translate encode(): comments present not transferred, no sequence output";
    }
  }

  //warn << "encode() ret:\n" << ret.dump();
  return ret;
}

}  // anon namespace


/**
 * @brief Insert markers for initialization code
 */
void insertInitBlock(Instr::List &code) {
  int index = code.lastUniformOffset();
  Instr::List ret;

  ret << Instr(INIT_BEGIN)
      << Instr(INIT_END);

  code.insert(index + 1, ret);
}


/**
 * @brief Insert the init block code into the Target program
 *
 * Also adds header comments for the init block and the main program
 * in the Target code.
 *
 * @param code  program compiled to Target language
 * @param init   list of operations for the init block
 */
void insert_init_block(Instr::List &code, Instr::List &init) {
  init.front().header("Init block");

  int insert_index = code.tag_index(INIT_BEGIN);
  assertq(insert_index >= 0, "Expecting init begin marker");
  code.insert(insert_index + 1, init);  // Insert init code after the INIT_BEGIN marker

  insert_index = code.tag_index(INIT_END);
  assertq(insert_index >= 0, "Expecting init end marker");
  code[insert_index + 1].header("Main program");
}


// ============================================================================
// Interface
// ============================================================================

/**
 * @brief Encode variable assignments to Target code
 *
 * Translates the (conditional) assignment of a variable to an expression.
 *
 * @param cond  Condition on assignment
 * @param v     Variable on lhs
 * @param expr  Expression on rhs
 *
 * @return  A sequence of instructions
 */
Instr::List varAssign(AssignCond cond, Var v, Expr::Ptr expr) {
  using namespace V3DLib::Target::instr;
  Instr::List ret;
  Expr e = *expr;


  switch (e.tag()) {
    case Expr::VAR: {                                                // 'v := w', v and w variables
        auto tmp = mov(v, e.var());
        assert(tmp.size() ==1);
        tmp.back().cond(cond);

        ret << tmp; 
      }
      break;
    case Expr::INT_LIT:                                              // 'v := i', i is an integer literal
      //Log::warn << "varAssign() c,expr: " << v.dump() << ", " << expr->dump();
      ret << li(v, e.intLit).cond(cond);
      break;
    case Expr::FLOAT_LIT:                                            // 'v := f', f is a float literal
      ret << li(v, e.floatLit).cond(cond);
      break;
    case Expr::APPLY: {                                              // 'v := x op y'
      if (!e.lhs()->isSimple() || !e.rhs()->isSimple()) {            // x or y are not simple
        e.lhs(simplify(&ret, e.lhs()));
        e.rhs(simplify(&ret, e.rhs()));
      }

      if (e.lhs()->isLit() && e.rhs()->isLit()) {                    // x and y are both literals
        Var tmpVar = VarGen::fresh();
        ret << varAssign(cond, tmpVar, e.lhs());
        e.lhs(mkVar(tmpVar));
      }

      switch (e.apply_op().op) {                                     // x and y are simple
        case RECIP:     ret << recip(v, e.lhs()->var());     break;
        case RECIPSQRT: ret << recipsqrt(v, e.lhs()->var()); break;
        case EXP:       ret << bexp(v, e.lhs()->var());      break;
        case EXP_E:     ret << bexp_e(v, e.lhs()->var());    break;
        case LOG:       ret << blog(v, e.lhs()->var());      break;

        default:
          // Everything else is considered to be a single binary operation
          Instr instr(ALU);
          instr.ALU.op    = ALUOp(e.apply_op());
          instr.ALU.srcA  = operand(e.lhs());
          instr.ALU.srcB  = operand(e.rhs());
          instr.assign_cond(cond);
          instr.dest(v);

          ret << instr;
        break;
      }
    }
    break;
    case Expr::DEREF:                                                // 'v := *w'
      if (e.deref_ptr()->tag() != Expr::VAR) {                       // w is not a variable
        assert(!e.deref_ptr()->isLit());
        e.deref_ptr(simplify(&ret, e.deref_ptr()));
      }
                                                                     // w is a variable
      //
      // Restriction:
      // dereferencing is disallowed in conditional ('where') assignments for simplicity.
      // In most (all?) cases it should be trivial to lift these outside the 'where'.
      //
      assertq(cond.is_always(), "V3DLib: dereferencing not yet supported inside 'where'");
      ret << getSourceTranslate().load_var(v, e);
      break;
    default:
      assertq(false, "This case should not be reachable");
      break;
  }

  return ret;
}


Instr::List varAssign(Var v, Expr::Ptr expr) {
  return varAssign(always, v, expr);  // TODO: For some reason, `always` *must* be passed in.
                                      //       Overloaded call generates segfault
}


/**
 * Similar to 'simplify' but ensure that the result is a variable.
 */
Expr::Ptr putInVar(Instr::List *seq, Expr::Ptr e) {
  Log::cdebug << "Called PutInVar()";

  if (e->tag() == Expr::VAR) {
    return e;
  }

  Var tmp = VarGen::fresh();
  *seq << varAssign(tmp, e);
  return mkVar(tmp);
}


/**
 * @brief Translate a list of Source statements to Target code
 *
 * Entry point for translation of statements.
 * Is also called recursively internally.
 *
 * @param target write-parameter; list of converted Target statements
 * @param source read-parameter; list of Source statements to convert 
 */
void encode_target(Instr::List &target, Stmt::Array const &source) {
  assert(!source.empty());

  for (int i = 0; i < (int) source.size(); i++) {
    auto ret = encode(source[i]);
    target << ret;
/*
    auto src_tmp = source[i]->dump();
    auto ret_tmp = ret.dump();
    warn << "encode_target() translated source op:\n"
         << "  Source:"
         << ((num_newlines(src_tmp) <= 1)?" ":"\n")
         << src_tmp << "\n"
         << "  Target:"
         << ((num_newlines(ret_tmp) <= 1)?" ":"\n")
         << ret_tmp;
*/
  }
}

}  // namespace V3DLib
