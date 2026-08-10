#include "Source/Float.h"
#include "Lang.h"  // only for assign()!
#include "Functions.h"

namespace V3DLib {

FloatExpr unary_float_op(OpId op_id, FloatExpr a) {
  Expr::Ptr dummy = mkVar(Var(DUMMY));
  Expr::Ptr e = mkApply(a.expr(), Op(op_id, FLOAT), dummy);
  return FloatExpr(e);
}


// ============================================================================
// Class FloatExpr
// ============================================================================

FloatExpr::FloatExpr(float x) {
  m_expr = std::make_shared<Expr>(x);
}


FloatExpr::FloatExpr(Deref<Float> d) : BaseExpr(d.expr()) {}

FloatExpr FloatExpr::operator-() { return (*this) * -1.0f; }


// ============================================================================
// Class Float
// ============================================================================

/**
 * Default ctor
 *
 * This is required for the creation of temp variables in expressions.
 *
 * There is an issue with initialization of variables without a value in the code, e.g.:
 *
 *     Float a;
 *
 * Variables need an explicit init, e.g.  '= 0', otherwise var will not be created and compilation fails.
 * This is slightly bothersome when using it in templates where the var type is a template parameter,
 * e.g. type could also be Complex.
 *
 * I have not found a resolution for this yet, hoping to fix it eventually.
 */
Float::Float() { assign_intern(); }


Float::Float(float x) {
  auto a = std::make_shared<Expr>(x);
  assign_intern(a);
}


Float::Float(FloatExpr e)    { assign_intern(e.expr()); }
Float::Float(Deref<Float> d) { assign_intern(d.expr()); }
Float::Float(Float const &x) { assign_intern(x.expr()); }


bool Float::passParam(IntList &uniforms, float val) {
  int32_t* bits = (int32_t*) &val;
  uniforms.append(*bits);
  return true;
}


uint32_t Float::param_value(float val) {
  int32_t* bits = (int32_t*) &val;
  return *bits;
}


/**
 * Cast to a FloatExpr
 */
Float::operator FloatExpr() const { return FloatExpr(m_expr); }


/** @addtogroup SourceLanguage
 *  @{
 */

/**
 * @brief Reinterpret the incoming Int expression as Float.
 *
 * This is a bitwise conversion; whatever the bits of the integer expessions are,
 * they are reinterpreted as a Float.
 */
void Float::as_float(IntExpr rhs) {
  (*this) = FloatExpr(rhs.expr());
}


/**
 * @brief Reinterpret the Float value as an Int expression.
 *
 * This is a bitwise conversion; whatever the bits of the Float variable are,
 * they are reinterpreted as an Int.
 */
IntExpr Float::as_int() const {
  return IntExpr(m_expr);
}


// ============================================================================
// Assignments
// ============================================================================

/**
 * @brief Assign a float constant to a Float variable.
 *
 * Usage example:
 *
 *     Float a = 1.23f;
 *     // Variable a now contains: <1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f,1.23f>
 */
Float &Float::operator=(float rhs) {
  Float tmp(rhs);
  assign(m_expr, tmp.expr());
  return self();
}

/** @} */ // end of group SourceLanguage


Float &Float::operator=(Float &rhs) {
  assign(m_expr, rhs.expr());
  return rhs;
}

Float &Float::operator=(Float const &rhs) {
  assign(m_expr, rhs.expr());
  return self();
}


FloatExpr Float::operator=(FloatExpr const &rhs) {
  assign(m_expr, rhs.expr());
  return self();
}


Float &Float::operator=(Deref<Float> d) {
  Float tmp = d;
  assign(m_expr, tmp.expr());
  return self();
}


Float &Float::self() {
  return *(const_cast<Float *>(this));
}


Float &Float::operator+=(FloatExpr rhs) { *this = *this + rhs; return *this; }
Float &Float::operator-=(FloatExpr rhs) { *this = *this - rhs; return *this; }
Float &Float::operator*=(FloatExpr rhs) { *this = *this * rhs; return *this; }


// ============================================================================
// Generic operations
// ============================================================================

inline FloatExpr mkFloatApply(FloatExpr lhs, Op const &op, FloatExpr rhs) {
  Expr::Ptr e = mkApply(lhs.expr(), op, rhs.expr());
  return FloatExpr(e);
}


inline FloatExpr mkFloatApply(FloatExpr rhs, Op const &op) {
  Expr::Ptr e = mkApply(rhs.expr(), op);
  return FloatExpr(e);
}


/**
 * @brief Read a Float from the UNIFORM FIFO.
 */
Float Float::mkArg() {
  Expr::Ptr e = mkVar(UNIFORM);
  Float x;
  x = FloatExpr(e);
  return x;
}


void Float::set_at(Int n, Float const &src) {
  V3DLib::set_at(*this, n, src);
}

// ============================================================================
// Operations
// ============================================================================

/**
 * Read vector from VPM
 */
FloatExpr vpmGetFloat() {
  Expr::Ptr e = mkVar(VPM_READ);
  return FloatExpr(e);
}


/**
 * Vector rotation for float values
 */
FloatExpr rotate(FloatExpr a, IntExpr b) {
  Expr::Ptr e = mkApply(a.expr(), Op(ROTATE, FLOAT), b.expr());
  return FloatExpr(e);
}


/**
 * Conversion to Int
 */
IntExpr toInt(FloatExpr a) {
  Expr::Ptr e = mkApply(a.expr(), Op(FtoI, INT32));
  return IntExpr(e);
}


/**
 * @brief Conversion to Float
 */
FloatExpr toFloat(IntExpr a) {
  Expr::Ptr e = mkApply(a.expr(), Op(ItoF, FLOAT));
  return FloatExpr(e);
}


/**
 * Implementation of fabs() in source language.
 *
 * Relies on IEEE 754 specs for 32-bit floats.
 * Special values (Nan's, Inf's) are ignored
 */
FloatExpr fabs(FloatExpr x) {
  uint32_t const Mask = ~(((uint32_t) 1) << 31);

  // Just zap the top bit
  Float ret;
  ret.as_float(x.as_int() & Mask);
  return ret;
}


FloatExpr operator+(FloatExpr a, FloatExpr b) { return mkFloatApply(a, Op(ADD, FLOAT), b); }
FloatExpr operator-(FloatExpr a, FloatExpr b) { return mkFloatApply(a, Op(SUB, FLOAT), b); }
FloatExpr operator*(FloatExpr a, FloatExpr b) { return mkFloatApply(a, Op(MUL, FLOAT), b); }

FloatExpr operator/(FloatExpr a, FloatExpr b) {
  return mkFloatApply(a, Op(MUL, FLOAT), recip(b));
}

FloatExpr min(FloatExpr a, FloatExpr b)       { return mkFloatApply(a, Op(MIN, FLOAT), b); }
FloatExpr max(FloatExpr a, FloatExpr b)       { return mkFloatApply(a, Op(MAX, FLOAT), b); }


/**
 * @addtogroup SourceLanguage
 *
 * ### SFU functions
 *
 * The `SFU` (Special Function Unit) is a separate hardware component on
 * `vc4` and `vc6`, which runs specific functions.  
 * The functions in question are delegated to the `SFU`. Any `SFU` operation takes
 * two program cycles to complete and the result is put in special
 * accumulator register `ACC4`.
 *
 * On `vc7`, there is no `SFU`. The special functions have been reassigned as
 * regular hardware operations on the Add ALU.
 *
 *  @{
 */

/**
 * @brief For the Float parameter `x` return `1/x`.
 *
 * This is an `SFU` operation.
 */
FloatExpr recip(FloatExpr x)     { return mkFloatApply(x, Op(RECIP    , FLOAT)); }


/**
 * @brief For the Float parameter `x` return `1/sqrt(x)`.
 *
 * This is an `SFU` operation.
 */
FloatExpr recipsqrt(FloatExpr x) { return mkFloatApply(x, Op(RECIPSQRT, FLOAT)); }


/**
 * @brief For the Float parameter `x` return `2^x`.
 *
 * Note that the base is `2`.
 *
 * This is an `SFU` operation.
 */
FloatExpr exp(FloatExpr x)       { return mkFloatApply(x, Op(EXP      , FLOAT)); }


/**
 * @brief For the Float parameter `x` return `e^x`.
 *
 * Note that the base is `e`.
 *
 * This is a library function which uses `SFU` operation `exp()`.
 */
FloatExpr exp_e(FloatExpr x)     { return mkFloatApply(x, Op(EXP_E    , FLOAT)); }


/**
 * @brief For the Float parameter `x` return `tanh(x)`.
 *
 * This is a library function which internally uses `SFU` operation `exp()`.
 */
FloatExpr tanh(FloatExpr x)      { return mkFloatApply(x, Op(TANH     , FLOAT)); }


/**
 * @brief For the Float parameter `x` return `log(x)`.
 *
 * Note that the base is `2`.
 *
 * This is an `SFU` operation.
 */
FloatExpr log(FloatExpr x)       { return mkFloatApply(x, Op(LOG      , FLOAT)); }


/**
 * @brief For the Float parameter `x` return `ln(x)`.
 *
 * Note that the base is `e`.
 *
 * This is a library function which uses `SFU` operation `log()`.
 */
FloatExpr ln(FloatExpr x)        { return mkFloatApply(x, Op(LOG_E    , FLOAT)); }


/**
 * @brief For the Float parameter `x` return `sqrt(x)`.
 *
 * This is a library function which internally uses `SFU` operations `recip()` and `recipsqrt()`.
 * There is no direct `sqrt` operation on the `QPU`'s.
 *
 * The `_f` postfix is added to avoid conflicts with other `sqrt` functions.
 * There might be a better solution for this.
 */
FloatExpr sqrt_f(FloatExpr x) { return recip(recipsqrt(x)); }

/** @} */ // end of group SourceLanguage


/**
 * Should not be used directly in code.
 * use `sin()` instead.
 *
 * Made visible for use in `Functions.cpp`.
 */
FloatExpr sin_op(FloatExpr x) {
  return unary_float_op(SIN, x);
}

namespace {

/**
 * Used for `v3d`, defined as function for `vc4`.
 */
FloatExpr ffloor_op(FloatExpr a) { return unary_float_op(FFLOOR, a); }

};


/** @addtogroup SourceLanguage
 *  @{
 */


/**
 * @brief Conversion unsigned to Float
 *
 * Int is a signed value by syntax, here it is regarded as unsigned.
 */
FloatExpr UnsignedtoFloat(IntExpr a) {
  using namespace functions;

  if (Platform::compiling_for_vc4()) {
    return vc4::u_to_f(a);
  } else {
    return v3d::u_to_f(a);
  }
}


/**
 * Implementation of floor() in source language.
 *
 * `v3d` has a hardware floor() operation, and is therefore a one-liner.
 */
FloatExpr ffloor(FloatExpr x) {
  using namespace functions;

  Float ret;

  if (Platform::compiling_for_vc4()) {
    ret = vc4::ffloor(x);
  } else {
    // v3d
    ret = ffloor_op(x);  comment("ffloor() v3d");
  }

  return ret;
}


/**
 * @brief Source-level call for `cos()`.
 *
 * @param x Angle in units of `2*PI`. Hence `x_in = 0.5f` stands for `PI`. 
 *
 */
FloatExpr cos(FloatExpr x) {
  using namespace functions;

  if (Platform::compiling_for_vc4()) {
    return vc4::cos(x);
  } else {
    return v3d::sin(0.25f - x);
  }
}


/**
 * @brief Source-level call for `sin()`.
 *
 * @param x Angle in units of `2*PI`. Hence `x_in = 0.5f` stands for `PI`. 
 *
 */
FloatExpr sin(FloatExpr x) {
  using namespace functions;

  if (Platform::compiling_for_vc4()) {
    return vc4::sin(x);
  } else {
    return v3d::sin(x);
  }
}

/** @} */ // end of group SourceLanguage

}  // namespace V3DLib

