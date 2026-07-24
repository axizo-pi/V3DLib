#include "Int.h"
#include "Lang.h"       // only for assign()!
#include "Support/Platform.h"
#include "Support/debug.h"
#include "Functions.h"  // operator/

/** @defgroup SourceLanguage Source Language Operations.
 *
 * This page will document all operations which can be used in a kernel
 * at Source-level. Ie., the operations that can be used when defining
 * a kernel at the top level.
 *
 * **NOTE:** Ongoing work, far from complete.
 */

namespace V3DLib {

using ::operator<<;  // C++ weirdness

// ============================================================================
// Class Int
// ============================================================================

Int::Int() { assign_intern(); }

Int::Int(int x) {
  //Log::warn << "Int ctor val: " << x;
  assign_intern(mkIntLit(x));
}

Int::Int(Deref<Int> d) {
  //Log::warn << "Int ctor deref: " << d.expr()->dump();
  assign_intern(d.expr());
}

Int::Int(IntExpr e) {
  //Log::warn << "Int ctor intexpr";
  assign_intern(e.expr());
}

Int::Int(Int const &x) {
  //Log::warn << "Int ctor int const";
  assign_intern(x.expr());
}


/**
 * @brief Cast to an IntExpr.
 */
Int::operator IntExpr() const {
  return IntExpr(m_expr);
}


/** @addtogroup SourceLanguage
 *  @{
 */

/**
 * @brief Assign an integer constant to an Int variable.
 *
 * Usage example:
 *
 *     Int val = 123;
 */
Int &Int::operator=(int x) {
  Int tmp(x);
  (*this) = tmp;
  return *this;
}

/** @} */ // end of group SourceLanguage

Int &Int::operator=(Int const &rhs) {
  assign(m_expr, rhs.expr());
  return *this; //rhs;
}

IntExpr Int::operator=(IntExpr rhs) {
  assign(m_expr, rhs.expr());
  return rhs;
}


Int &Int::operator+=(IntExpr rhs) { *this = *this + rhs; return *this; }
Int &Int::operator-=(IntExpr rhs) { *this = *this - rhs; return *this; }
Int &Int::operator|=(IntExpr rhs) { *this = *this | rhs; return *this; }
Int &Int::operator/=(IntExpr rhs) { *this = *this / rhs; return *this; }
Int &Int::operator*=(IntExpr rhs) { *this = *this * rhs; return *this; }


//
// Note that these do not follow the c-convention that inc/dec
// is performed AFTER current expression.
//
void Int::operator++(int) { *this = *this + 1; }
void Int::operator--(int)  { *this = *this - 1; }


// ============================================================================
// Generic operations
// ============================================================================

inline IntExpr mkIntApply(IntExpr a, Op const &op, IntExpr b) {
  Expr::Ptr e = mkApply(a.expr(), op, b.expr());
  return IntExpr(e);
}


// ============================================================================
// Specific operations
// ============================================================================

/**
 * Read an Int from the UNIFORM FIFO.
 */
IntExpr getUniformInt() {
   Expr::Ptr e = std::make_shared<Expr>(Var(UNIFORM));
  return IntExpr(e);
}


Int Int::mkArg() {
  Int x;
  x = getUniformInt();
  return x;
}


bool Int::passParam(IntList &uniforms, int val) {
  uniforms.append((int32_t) val);
  return true;
}


uint32_t Int::param_value(int val) {
  return (int32_t) val;
}

/** @addtogroup SourceLanguage
 *  @{
 */

/**
 * @brief Return a vector containing integers 0..15
 *
 * On `vc4` this is a special register, on `v3d` this is an instruction.
 */
IntExpr index() {
  if (Platform::compiling_for_vc4()) {
    Expr::Ptr e = std::make_shared<Expr>(Var(ELEM_NUM));
    return IntExpr(e);
  } else {
    Expr::Ptr a = mkVar(Var(DUMMY));
    Expr::Ptr e = mkApply(a, Op(EIDX, INT32), a);
    return IntExpr(e);
  }
}



/**
 * @brief Return a vector containing the QPU id.
 *
 * Implemented as a reserved variable.
 */
IntExpr me() {
  Expr::Ptr e = std::make_shared<Expr>(Var(STANDARD, RSV_QPU_ID));
  return IntExpr(e);
}


/**
 * @brief Return a vector containing the QPU count.
 *
 * Implemented as a reserved variable.
 */
IntExpr numQPUs() {
  Expr::Ptr e = std::make_shared<Expr>(Var(STANDARD, RSV_NUM_QPUS));
  return IntExpr(e);
}

/** @} */ // end of group SourceLanguage


/**
 * Read vector from VPM
 */
IntExpr vpmGetInt() {
  Expr::Ptr e = std::make_shared<Expr>(Var(VPM_READ));
  return IntExpr(e);
}


/**
 * Vector rotation for int values
 */
IntExpr rotate(IntExpr a, IntExpr b) {
  return mkIntApply(a, Op(ROTATE, INT32), b);
}


/**
 * Count Leading Zeroes
 */
IntExpr clz(IntExpr a) {
  return mkIntApply(a, Op(CLZ, INT32), a);
}


IntExpr operator+(IntExpr a, IntExpr b)  { return mkIntApply(a, Op(ADD,  INT32), b); }
IntExpr operator-(IntExpr a, IntExpr b)  { return mkIntApply(a, Op(SUB,  INT32), b); }
IntExpr operator*(IntExpr a, IntExpr b)  { return mkIntApply(a, Op(MUL,  INT32), b); }
IntExpr operator<<(IntExpr a, IntExpr b) { return mkIntApply(a, Op(SHL,  INT32), b); }
IntExpr operator>>(IntExpr a, IntExpr b) { return mkIntApply(a, Op(SHR,  INT32), b); }
IntExpr operator&(IntExpr a, IntExpr b)  { return mkIntApply(a, Op(BAND, INT32), b); }
IntExpr operator|(IntExpr a, IntExpr b)  { return mkIntApply(a, Op(BOR,  INT32), b); }
IntExpr operator^(IntExpr a, IntExpr b)  { return mkIntApply(a, Op(BXOR, INT32), b); }
IntExpr operator~(IntExpr a)             { return mkIntApply(a, Op(BNOT, INT32), a); }  //Translated to move;  Should be same as bnot()
IntExpr min(IntExpr a, IntExpr b)        { return mkIntApply(a, Op(MIN,  INT32), b); }
IntExpr max(IntExpr a, IntExpr b)        { return mkIntApply(a, Op(MAX,  INT32), b); }
IntExpr shr(IntExpr a, IntExpr b)        { return mkIntApply(a, Op(USHR, INT32), b); }
IntExpr ror(IntExpr a, IntExpr b)        { return mkIntApply(a, Op(ROR,  INT32), b); }
IntExpr bnot(IntExpr a)                  { return mkIntApply(a, Op(BNOT, INT32), a); }  // TODO: NOT WORKING; always returns 0


/**
 * Return division of values
 *
 * Integer division is costly; should you need both quotient and remainder,
 * It is better to call integer_division() directly.
 */
IntExpr operator/(IntExpr a, IntExpr b) {
  // b == 0 a bad idea, assert() won't work for testing
  using functions::integer_division;
  Int quotient;
  Int remainder;
  integer_division(quotient, remainder, a, b);
  return quotient;
}
/**
 * Return remainder of values
 *
 * Integer division is costly; should you need both quotient and remainder,
 * It is better to call integer_division() directly.
 */
IntExpr operator%(IntExpr a, IntExpr b) {
  // b == 0 a bad idea, assert() won't work for testing
  using functions::integer_division;
  Int quotient;
  Int remainder;
  integer_division(quotient, remainder, a, b);
  return remainder;
}

}  // namespace V3DLib
