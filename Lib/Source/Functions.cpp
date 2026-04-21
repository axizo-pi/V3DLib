/******************************************************************************
 * Function Library for functions at the source language level
 *
 * These are not actual functions but generate inlined code.
 * In the kernel code, though, they look like function calls.
 *
 ******************************************************************************/
#include "Functions.h"
#include "Lang.h"
#include "vc4/DMA/VPMArray.h"
#include "LibSettings.h"
#include <cmath>

namespace V3DLib {
namespace functions {
namespace {

int const MAX_INT = 2147483647;  // Largest positive 32-bit integer that can be negated

} // anon namespace


/**
 * Ensure a common exit method for function snippets.
 *
 * The return value should actually be an IntExpr instance, which is derived downstream
 * from the `dummy` variable as defined here.
 *
 * This is more of a semantics thing; it's a tiny bit of code, but the name implies
 * what the intention is.
 */
void Return(Int const &val) {
  // Prepare an expression which can be assigned
  // dummy is not used downstream, only the rhs matters
  Int dummy;
  dummy = val;
}


void Return(Float const &val) {
  // Prepare an expression which can be assigned
  // dummy is not used downstream, only the rhs matters
  Float dummy;
  dummy = val;
}


/**
 * Create a function snippet from the generation of the passed callback
 *
 * This hijacks the global statement stack to generate from source lang,
 * and then isolates the generation in a separate expression.
 *
 * The immediate benefit of this is to be able to define source lang
 * constructs using the source lang itself.

 * This can be done to some extent directly, but defining them as standalone code
 * is more flexible.
 * The code snippets are relocatable and can be inserted anywhere
 *
 * Potential other uses:
 *   - memoization
 *   - true functions (currently everything generated inline)
 *
 * Because this uses the global statement stack, it is **not** threadsafe.
 * But then again, nothing using the global statement stack is.
 */
IntExpr create_function_snippet(StackCallback f) {
  auto stmts = tempStmt(f);
  assert(!stmts.empty());
  stmtStack() << stmts;
  Stmt::Ptr ret = stmts.back();
  return ret->assign_rhs();
}


// TODO see if this can be merged with the Int version.
FloatExpr create_float_function_snippet(StackCallback f) {
  auto stmts = tempStmt(f);

  assert(!stmts.empty());

  // Return only the assign part of the final statement and remove that statement
  auto stmt = stmts.back();
  stmts.pop_back();
  stmtStack() << stmts;
  return stmt->assign_rhs();
}


/**
 * This is the same as negation.
 *
 * Used as an alternative for `-1*a`, because vc4 does 24-bit multiplication only.
 */
IntExpr two_complement(IntExpr a) {
  return create_function_snippet([a] {
    Int tmp = a;
    tmp = (tmp ^ -1) + 1;  // take the 1's complement

    Return(tmp);
  });
}


/**
 * Return the biggest possible integer. This is 0x7fffff.
 */
IntExpr _INF() {
  return create_function_snippet([] {

    Int tmp = 4;      comment("Load INF");  // Important that comment is AFTER first statement
    tmp = tmp << 15;
    tmp = tmp << 14;
    tmp = (tmp ^ -1);  // -1 = 0xffffffff

    Return(tmp);
  });
}


IntExpr abs(IntExpr a) {
  return create_function_snippet([a] {
    Int tmp = a;

    Where (tmp < 0)
      tmp = (tmp ^ 0xffffffff) + 1;  // take the 1's complement
    End

    Return(tmp);
  });
}


/**
 * Determine index of topmost bit set.
 *
 * Incoming values are assumed to be unsigned.
 * a == 0 will return -1.
 */
IntExpr topmost_bit(IntExpr in_a) {
  return create_function_snippet([in_a] {
    Int a = in_a;
    Int topmost = -1;

    For (Int n = 30, n >= 0, n--)
      Where (topmost == -1)
        Where ((a & (1 << n)) != 0)
          topmost = n;
        End
      End
    End

    Return(topmost);
  });
}


/**
 * Long integer division, returning quotient and remainder
 *
 * There is no support for hardware integer division on the VideoCores, this is an implementation for
 * when you really need it (costly).
 *
 * Source: https://en.wikipedia.org/wiki/Division_algorithm#Integer_division_(unsigned)_with_remainder
 */
void integer_division(Int &Q, Int &R, IntExpr in_a, IntExpr in_b) {
  Int N = in_a;  comment("Start long integer division");
  Int D = in_b;

  Int sign = 1;

  Where ((N >= 0) != (D >= 0))       // Determine sign
    sign = -1;
  End
 
  N = abs(N);
  D = abs(D);

  Q = 0;                             // Initialize quotient and remainder to zero
  R = 0;

  IntExpr top_bit = topmost_bit(N);  // Find first non-zero bit


  For (Int i = 30, i >= 0, i--)
    Where (D == 0)
      //Q = MAX_INT;                   // Indicates infinity
      Q = _INF();
    Else
      Where (top_bit >= i)
        R = R << 1;                  // Left-shift R by 1 bit (lsb == 0)
        R |= (N >> i) & 1;           // Set the least-significant bit of R equal to bit i of the numerator
        Where (R >= D)
          R -= D;
          Q |= (1 << i);
        End
      End
    End
  End

  Where (sign == -1)
    Q = two_complement(Q);
  End
 
  comment("End long integer division");
}


/**
 * Do integer division by converting to and from float.
 *
 * This is not always precise (confirmed) but more concise than the full integer calculation
 */
IntExpr integer_division_f(IntExpr in_a, IntExpr in_b) {
  return create_function_snippet([in_a, in_b] {
    Float a = toFloat(in_a);    comment("Start integer division by float");
    Float b = toFloat(in_b);

    Int res;

    Where (in_b == 0)
      res = _INF();
    Else
      // Doing it like this (all in one line) leads to multiple generations of
      // this function in the output. No clue why.
      // res = toInt(functions::ffloor(a / b));  // ffloor() fixes rounding up 

      // This works as expected
      Float tmp = a/b;
      tmp = functions::ffloor(tmp);  // ffloor() fixes rounding up 
      res = toInt(tmp);
    End

    comment("End integer division by float");

    return Return(res);
  });
}


///////////////////////////////////////////////////////////////////////////////
// Trigonometric functions
///////////////////////////////////////////////////////////////////////////////

namespace scalar {

/**
 * scalar version of cosine.
 *
 * This circumvents the QPU cos() function.
 *
 * The input param is normalized on 2*M_PI. Hence setting `x = 1.0f` means that
 * `cos(2*M_PI) is calculated.
 *
 * From source:
 *   If EXTRA_PRECISION is defined, the maximum error is about 0.00109 for the range -π to π,i
 *   assuming T is double. Otherwise, the maximum error is about 0.056 for the same range.
 *
 * Source: https://stackoverflow.com/questions/18662261/fastest-implementation-of-sine-cosine-and-square-root-in-c-doesnt-need-to-b/28050328#28050328
 */
float cos(float x_in, bool extra_precision) noexcept {
  double x = x_in;
  
  // setting to true in param overrides lib setting
  extra_precision |= LibSettings::use_high_precision_sincos();


  x -= .25 + std::floor(x + .25);
  x *= 16. * (std::abs(x) - .5);

  if (extra_precision) {
    //Log::warn << "doing extra precision 2";
    x += .225 * x * (std::abs(x) - 1.0f);
  }

  return (float) x;
}


/**
 * scalar version of sine
 *
 * NB: The input param is normalized on 2*M_PI.
 */
float sin(float x_in, bool extra_precision) noexcept {
  return functions::scalar::cos(0.25f - x_in, extra_precision);
}

} // namespace scalar


/**
 * This circumvents the SFU cos() function.
 *
 * See header comment of scalar cos().
 *
 * Possible alternative: https://www.johndcook.com/blog/2021/03/21/simple-trig-approx/
 */
FloatExpr cos_prev(FloatExpr x_in, bool extra_precision) {
  Float x = x_in;

  x -= 0.25f + functions::ffloor(x + 0.25f);  comment("Start cosine");
  x *= 16.0f * (fabs(x) - 0.5f);

  extra_precision |= LibSettings::use_high_precision_sincos();
  if (extra_precision) {
    //Log::warn << "doing extra precision";
    x += 0.225f * x * (fabs(x) - 1.0f);
  }

  return x;
}


/**
 * @brief cosine for QPU using Taylor approximation.
 *
 * This is _much_ more precise than the previous functions `cos_prev()` (see above).  
 * `vc4` has no cosine, hence an explicit implementation is needed.
 *
 * @param x_in angle in units of 2*M_PI. Hence `x_in = 0.5f` stands for 1M_PI`. 
 *
 * Source: https://www.numberanalytics.com/blog/ultimate-taylor-trigonometry-guide#series-for-sine-and-cosine
 */
FloatExpr cos(FloatExpr x_in) {
  // Empirically determined interval for zero
  Float ZERO_MIN = -1.26078e-06f; 
  Float ZERO_MAX =  4.24525e-08f;

  Float x = x_in;

  // Normalize x to a value in the range [-0.5, 0.5]
  Float tmp = x + 0.5f;
  x = tmp - functions::ffloor(tmp) - 0.5f;

  x = x * (float) (2.0f * M_PI);  comment("Start Taylor");

  Float x_sqr      = x*x;
  Float divisor    = 1;
  int   iterations = 8;           // Smallest value that passes all unit tests

  Float ret         = 1.0f;
  Float coefficient = 1.0f;       comment("Start Loop");

  for (int i = 0; i < iterations; ++i) {
    divisor     *= 1.0f/((float) ((2*i + 1)*(2*i + 2)));
    coefficient *= x_sqr;
     
    if (i % 2 == 0) {
      ret  -= coefficient*divisor;
    } else {
      ret  += coefficient*divisor;
    }
  }

  // Adjust very small values to zero
  Where (ZERO_MIN < ret && ret < ZERO_MAX)
    ret = 0.0f;
  End

  return ret;
}


FloatExpr sin(FloatExpr x_in) {
  return functions::cos(0.25f - x_in);
}


/**
 * Calculate sine for v3d using hardware
 * 
 * Use this for v3d only.
 *
 * v3d sin takes params in which are multiples of PI.
 * Also works only in range -PI/2..PI/2.
 *
 * Incoming values are multiples of 2*PI.
 *
 * ============================================================================
 * NOTES
 * =====
 *
 * * In `DotVector::dft_dot_product()`:
 *
 *    Complex tmp1(elements[i]*cos(param), elements[i]*sin(param));
 *
 *   ... without `create_function_snippet()`, the calculation was split as follows:
 *
 *     - calc var tmp for cos()
 *     - calc var tmp for sin()
 *     - call sin() for cos
 *     - mult cos result with elements[i]
 *     - call sin() for sin
 *     - mult sin result with elements[i]
 *
 *   I.e., the sin() calls are delayed till they are actually used.
 *
 *   IMO this is because the tmp calculation is added immediately to the statement stack.
 *   However, the addition of the sin() operation is delayed; it is returned as an expression and 
 *   added directly after the current function has returned.
 *
 *   It's a bit of a stretch of imagination to see this happening, but it's definitely possible.
 *
 *   Nice to see that the calculation works fine, even with this happening.
 *  
 *   ... with `create_function_snippet()`, the calculation has the expected order:
 *
 *     - calc var tmp for cos()
 *     - call sin() for cos
 *     - calc var tmp for sin()
 *     - call sin() for sin
 *     - mult cos result with elements[i]
 *     - mult sin result with elements[i]
 * 
 *   This is yet another reason for using function snippets.
 *
 *   Okay, that was real interesting.
 */
FloatExpr sin_v3d(FloatExpr x_in) {
  //Log::warn << "using v3d sin";

  return create_float_function_snippet([x_in] {
    Float tmp = x_in;                    comment("Start source lang v3d sin");

    tmp += 0.25f;                        // Modulo to range -0.25...0.75
    comment("v3d sin preamble to get param in the allowed range");

    tmp -= functions::ffloor(tmp);       // Get the fractional part
    tmp -= 0.25f;

    Where (tmp > 0.25f)                  // Adjust value to the range -PI/2...PI/2
      tmp = 0.5f - tmp;
    End

    tmp *= 2;                            // Convert to multiple of PI
    comment("End v3d sin preamble");

    Return(sin_op(tmp));
  });
}


/**
 * Implementation of ffloor() in source language.
 *
 * This is meant specifically for vc4; v3d actually has an ffloor operation.
 *
 * Relies on IEEE 754 specs for 32-bit floats.
 * Special values (Nan's, Inf's) are ignored
 */
FloatExpr ffloor(FloatExpr x) {
  return create_float_function_snippet([x] {
    Float ret;

    if (Platform::compiling_for_vc4()) {
      int const SIZE_MANTISSA = 23;

      Int exp = ((x.as_int() >> SIZE_MANTISSA) & ((1 << 8) - 1)) - 127;  comment("Calc exponent"); 
      Int fraction_mask = (1 << (SIZE_MANTISSA - exp)) - 1;
      Int frac = x.as_int() & fraction_mask;                             comment("Calc fraction"); 

      //
      // Clear the fractional part of the mantissa
      //
      // Helper for better readability
      //
      auto zap_mantissa  = [&fraction_mask] (FloatExpr x) -> FloatExpr {
        Float ret;
        ret.as_float(x.as_int() & ~(fraction_mask + 0));
        return ret;
      };

      ret = x;  // result same as input for exp > 23 bits and whole-integer negative values
      comment("Start ffloor()");

      Where (exp <= 23)                      // Doesn't work, expecting SEQ: comment("Start ffloor()");
        Where (x >= 1)
          ret = zap_mantissa(x);
        Else Where (x >= 0)
          ret = 0.0f;
        Else Where (x >= -1.0f)
          ret = -1.0f;
        Else Where (x < -1.0f && (frac != 0))
          ret = zap_mantissa(x) - 1;
        End End End End
      End
    } else {
      // v3d
      ret = V3DLib::ffloor(x);  comment("ffloor() v3d");
    }

    //return ret;
    return Return(ret);
  });
}


/**
 * Implementation of fabs() in source language.
 *
 * Relies on IEEE 754 specs for 32-bit floats.
 * Special values (Nan's, Inf's) are ignored
 */
FloatExpr fabs(FloatExpr x) {
  Float ret;

  if(Platform::compiling_for_vc4()) {
    uint32_t const Mask = ~(((uint32_t) 1) << 31);

    // Just zap the top bit
    ret.as_float(x.as_int() & Mask);            comment("fabs vc4");
  } else {
    // v3d: The conversion of Mask is really long-winded; make the mask in-place
    ret.as_float(x.as_int() & shr(Int(-1), 1));  comment("fabs v3d");
  }

  return ret;
}


}  // namespace functions


/**
 * Sum up all the vector elements of a register.
 *
 * All vector elements of register result will contain the same value.
 */
void rotate_sum(Int &input, Int &result) {
  result = input;              comment("rotate_sum");
  result += rotate(result, 1);
  result += rotate(result, 2);
  result += rotate(result, 4);
  result += rotate(result, 8);
}


void rotate_sum(Float &input, Float &result) {
  result = input;              comment("rotate_sum");
  result += rotate(result, 1);
  result += rotate(result, 2);
  result += rotate(result, 4);
  result += rotate(result, 8);
}


/**
 * Set value of src to vector element 'n' of dst
 *
 * All other values in dst are untouched.
 *
 * @param n  index of vector element to set. Must be in range 0..15 inclusive
 */
void set_at(Int &dst, Int n, Int const &src) {
  Where(index() == n)
    dst = src;
  End 
}


void set_at(Float &dst, Int n, Float const &src) {
  Where(index() == n)
    dst = src;
  End 
}


void mutex_acquire() {
  assert(Platform::compiling_for_vc4());

  Expr::Ptr dummy = mkVar(Var(DUMMY));
  Expr::Ptr mutex = mkVar(Var(MUTEX_ACQUIRE));  // Read A/B

  Stmt::Ptr ptr = Stmt::create_assign(dummy, mutex);
  stmtStack().push(ptr);
}


/**
 * Can't use DUMMY as src var. Fails on Target translation.
 */
void mutex_release() {
  assert(Platform::compiling_for_vc4());

  Expr::Ptr mutex = mkVar(Var(MUTEX_RELEASE));  // Write A/B

  Stmt::Ptr ptr = Stmt::create_assign(mutex, IntExpr(0).expr());
  stmtStack().push(ptr);
}


namespace {

/**
 * @brief barrier implementation for `vc4`
 *
 * This uses VPM as shared memory
 * You can't assume that `QPU 0` enters this routine first.
 *
 * ---------------------------------------------
 *
 * Notes
 * -----
 *
 * - This code is for `vc4`. `v3d` has a `barrier` operation, which
 *   is much more convenient.
 */
void barrier_vc4() {
  //Log::warn << "barrier vc4";
  assert(Platform::compiling_for_vc4());

  //
  // 'I' refers to the QPU that grabbed the mutex.
  // 'We' refers to all other QPU's participating.
  //

  nop(1);   header("vc4 barrier");

  If (numQPUs() != 1)                     // Don't bother if only one QPU
    Int mask = (1 << numQPUs()) - 1;

    vpm.set(0, 0);
    mutex_acquire();   comment("mutex_acquire");

    Int tmp = vpm.get(0);
    While (tmp != mask)
      vpm.set(0, tmp | (1<< me()));

      mutex_release();
      mutex_acquire();

      tmp = vpm.get(0);
    End

    mutex_release();

    //
    // There is a nonzero possibility  here in that the first released QPU's
    // overwrite vpm(0) with DMA transfers.
    //
    // Not apparent yet if I have to deal with that.
    // For the time being, assume that the loop is fast enough before any
    // DMA transfers happen.  
    // Also, DMA transfers use vertical layout, perhaps this is enough
    // to prevent any overwrites.
    //
  End
}


/**
 * @brief v3d-specific version of barrier
 *
 * Has no inputs, only an output, which is always magic reg SYNCB.
 *
 * `barrier` is v3d-specific. vc4 will need a different implementation,
 * most likely with semaphores.
 */
void barrier_v3d() {
  assertq(!Platform::compiling_for_vc4(), "This version of barrier runs only on v3d");
  stmtStack().push(Stmt::create(Stmt::BARRIER));
}

} // anon namespace


/**
 * @brief Generic version of `barrier()`
 */
void barrier() {
  if (Platform::compiling_for_vc4()) {
    // vc4 - Stmt::BARRIER is not passed on
    barrier_vc4();
  } else {
    barrier_v3d();
  }
}


void nop(int num) {
  assert(num >= 0);

  stmtStack() << Stmt::create(Stmt::NOP, nullptr, IntExpr(num).expr());
}

}  // namespace V3DLib
