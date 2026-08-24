#include "Int.h"
#include "Lang.h"             // only for assign()!
#include "Support/Platform.h"
#include "Support/debug.h"
#include "Functions.h"        // operator/

/** @defgroup SourceLanguage Source Language Operations.
 *
 * This page will document all operations which can be used in a kernel
 * at Source-language level.
 *
 * --------------------------
 *
 * ## Notes
 *
 * - **Ongoing work, far from complete.**
 * - It is worth noting that the hardware QPU's don't care if a value
 *   in a vector variable is Float or Int. A random Float hardware operation on
 *   an Int register will work just fine.  
 *   The difference between Int and Float is actually enforced in the Source language.
 *   Regard this as an attempt to retain your sanity when creating kernels.
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
 * @brief Set the Var id of Int instance to an existing Var.
 *
 * This is used to assign global constants in the library,
 * @see GlobalConstants.
 *
 * Do not use directly in kernel code.
 */
Int::Int(Var const &v)   { m_expr = mkVar(v); }


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
 *     Int a = 123;
 *     // Variable a now contains: <123,123,123,123,123,123,123,123,123,123,123,123,123,123,123,123>
 */
Int &Int::operator=(int x) {
  Int tmp(x);
  (*this) = tmp;
  return *this;
}


/**
 * @brief Assign The contents of variable `rhs` Int variable.

 * Usage example:
 *
 *     Int a = 123;
 *     Int b = a;
 */
Int &Int::operator=(Int const &rhs) {
  assign(m_expr, rhs.expr());
  return *this;
}

/** @} */ // end of group SourceLanguage


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
 * Example:
 *
 *     Int a = index();
 *     // Variable a now contains: <0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15>
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
 * @brief Return the id of the QPU currently running.
 *
 * The QPU is is a sequential value assigned to a QPU on execution of a kernel.
 * For example, if the number of QPU's is set to 4, there will be 4 QPU's running with
 * respectively `me()` == 0, 1, 2, 3.
 *
 * The number of QPU's running a kernel can be set with BaseKernel::setNumQPUs().
 *
 * Implemented as a reserved variable.
 */
IntExpr me() {
  Expr::Ptr e = std::make_shared<Expr>(Var(STANDARD, RSV_QPU_ID));
  return IntExpr(e);
}


/**
 * @brief Return the number of QPU's assigned to the execution of the current kernel.
 *
 * The number of QPU's running a kernel can be set with BaseKernel::setNumQPUs().
 *
 * Implemented as a reserved variable.
 */
IntExpr numQPUs() {
  Expr::Ptr e = std::make_shared<Expr>(Var(STANDARD, RSV_NUM_QPUS));
  return IntExpr(e);
}

/** @} */ // end of group SourceLanguage


/**
 * @brief Read vector from VPM
 *
 * VPM is used in conjunction with  DMA and is therefore intended for `vc4` only.
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


/** @addtogroup SourceLanguage
 *  @{
 */

/**
 * @brief Count leading zeroes
 *
 * Determine the number of zeroes before the first non-zero bit.
 *
 * This is a hardware operation for both `vc4` and `v3d`.
 *
 * Usage Example:
 *
 *      Int a = 1 << index();
 *      Int b = clz(a);
 *      // b contains: <31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16>
 */
IntExpr clz(IntExpr a) {
  return mkIntApply(a, Op(CLZ, INT32), a);
}


/**
 * @brief Generic version of `barrier()`
 */
void barrier() {
  using namespace functions;

  if (Platform::compiling_for_vc4()) {
    vc4::barrier();  // Stmt::BARRIER is not passed on for `vc4`
  } else {
    v3d::barrier();
  }
}

/** @} */ // end of group SourceLanguage


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
