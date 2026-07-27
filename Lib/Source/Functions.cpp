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

/**
 * /file
 *
 * Source-level functions.
 */

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
 * @param Return the integer value for infinity.
 *
 * For floats:
 * Infinity is defined as the largest possible exponent and a mantissa of zero.
 * This is 0x7f800000.
 * A non-zero mantissa with the largest exponent indicates NaN.
 *
 * However, this infinity value is used in integer calculations.
 * When INFINITY (C++ constant) is converted to integer, it becomes MAX_INT.
 */
IntExpr _INF() {
  return create_function_snippet([] {
   Int tmp = 4;      comment("Load integer INF");
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


/** @addtogroup SourceLanguage
 *  @{
 */

/**
 * @brief Determine index of topmost bit set.
 *
 * Can be used on any variable type, but using it on unsigned values make the most sense.
 *
 * Usage example:
 *
 *     Int  tmp = 1 << (index() + 3);
 *     Int b = topmost_bit(tmp);
 *     // b now contains: <3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18>
 *
 * This is basically the function of `clz`, which is a hardware operation.
 * Therefore, this operation is defined using `clz`.
 *
 * **TODO:** Consider removing.
 */
IntExpr topmost_bit(IntExpr in_a) {
  return create_function_snippet([in_a] {
    Int topmost = 31 - clz(in_a);

    Return(topmost);
  });
}


/**
 * @brief Long integer division, returning quotient and remainder.
 *
 * There is no support for hardware integer division on the VideoCores, this is an implementation for
 * when you really need it.
 *
 * The calculation is **exact**, however it is costly in terms of number of operations.  
 * If you are willing to trade precision for performance, consider using
 * `integer_division_f()` instead.
 *
 * Only call this directly is you need both quotient and remainder.
 * Usually, use the following source operations:
 *
 * - quotient only:  `IntExpr operator/(IntExpr a, IntExpr b)`
 * - remainder only: `IntExpr operator%(IntExpr a, IntExpr b)`
 *
 * Source: https://en.wikipedia.org/wiki/Division_algorithm#Integer_division_(unsigned)_with_remainder
 *
 * @param Q     Output variable; quotient result of the division
 * @param R     Output variable; remainder result of the division
 * @param num   Numerator of the division
 * @param denom Denominator of the division
 */
void integer_division(Int &Q, Int &R, IntExpr num, IntExpr denom) {
  Int N = num;    comment("Start long integer division");
  Int D = denom;

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
 * @brief Do integer division by converting to and from float.
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

/**
 * @brief Cosine for QPU using Taylor approximation.
 *
 * `vc4` has no cosine, hence an explicit implementation is needed.  
 * `v3d` has a hardware operation for cosine.
 *
 * @param x_in Angle in units of `2*PI`. Hence `x_in = 0.5f` stands for `PI`. 
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


/**
 * @brief Calculate sin value for given input.
 *
 * `sin()` is implemented in terms of `cos()`.
 *
 * @param x_in Angle in units of `2*PI`. Hence `x_in = 0.5f` stands for `M_PI`. 
 *
 */
FloatExpr sin(FloatExpr x_in) {
  return functions::cos(0.25f - x_in);
}


/**
 * @brief Calculate sine for v3d using hardware
 * 
 * **Normally, you do not need to call this function explicitly**.  
 * Use this for `v3d` only. The compilation will select this when appropriate.
 *
 * @param x_in Angle in units of `2*PI`. Works only in range `-PI/2..PI/2`.
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

/** @} */ // end of group SourceLanguage


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
 *
 * **TODO:** Taylor approximation is demonstrably better. Consider changing to Taylor.
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


//### End Trigonometric functions ###


/**
 * @brief Dissect a float value into the constituent fields
 *
 * This is defined as a partial.
 *
 * - sign == 1 when value is negative.
 * - The exponent is returned as the intended value.
 * - The significand needs to have the implied leading '1' to be proper.
 */
void float_fields(Float &x_f, Int &sign, Int &exponent, Int &significand) {
  int const SIZE_MANTISSA = 23;

  Int x = x_f.as_int();

  sign = (x >> 31) & 1;

  exponent = ((x >> SIZE_MANTISSA) & ((1 << 8) - 1)) - 127;

  Int fraction_mask = (1 << (SIZE_MANTISSA /*- exponent */)) - 1;
  significand = x & fraction_mask;
}


/**
 * Implementation of ffloor() in source language for vc4.
 *
 * Made visible for unit tests on v3d.
 *
 * Relies on IEEE 754 specs for 32-bit floats.
 * Special values (Nan's, Inf's) are ignored
 */
FloatExpr ffloor_vc4(FloatExpr x) {
  //warn << "Handling ffloor_vc4()";

  Float ret;

  Float x_val = x;
  Int sign;
  Int exp;
  Int significand;
  float_fields(x_val, sign, exp, significand);

  Int frac = (significand >> exp);

  //
  // Clear the fractional part of the mantissa
  //
  // Helper for better readability.
  //
  // TODO: freaking ugly and prob not necessary; see if can be cleaned up
  //
  auto zap_mantissa  = [&exp] (FloatExpr x) -> FloatExpr {
    int const SIZE_MANTISSA = 23;
    Int fraction_mask = (1 << (SIZE_MANTISSA - exp)) - 1;

    Float ret;
    ret.as_float(x.as_int() & ~(fraction_mask + 0));
    return ret;
  };

  ret = x;  // result same as input for exp > 23 bits and whole-integer negative values
  comment("Start ffloor()");

  Where (exp <= 23)
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

  return ret;
}


/**
 * Implementation of ffloor() in source language.
 *
 * `v3d` has a hardware ffloor operation, and is therefore a one-liner.
 */
FloatExpr ffloor(FloatExpr x) {
  //warn << "Handling ffloor()";

  Float ret;

  if (Platform::compiling_for_vc4()) {
    ret = ffloor_vc4(x);
  } else {
    // v3d
    ret = V3DLib::ffloor(x);  comment("ffloor() v3d");
  }

  return ret;
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


/**
 * @brief Calculate sum of the elements in the input vector
 *
 * Result is put in all the elements of the output vector
 *
 * Differences in calculation can, in fact, occur per element.
 * In the unit test, the least significant bit (bit 31) might be different.
 */
void rotate_sum(Float &input, Float &result) {
  result = input;              comment("rotate_sum");
  result += rotate(result, 1);
  result += rotate(result, 2);
  result += rotate(result, 4);
  result += rotate(result, 8);
}


/**
 * @brief Determine max value in the input vector
 *
 * Result is put in all the elements of the output vector
 */
void rotate_max(Float &input, Float &result) {
  Float tmp;

  result = input;              comment("rotate_max");
  tmp = rotate(result, 1);
  result = max(tmp, result);

  tmp = rotate(result, 2);
  result = max(tmp, result);

  tmp = rotate(result, 4);
  result = max(tmp, result);

  tmp = rotate(result, 8);
  result = max(tmp, result);
}


/**
 * @brief Determine min value in the input vector.
 *
 * Result is put in all the elements of the output vector.
 */
void rotate_min(Float &input, Float &result) {
  Float tmp;

  result = input;              comment("rotate_min");
  tmp = rotate(result, 1);
  result = min(tmp, result);

  tmp = rotate(result, 2);
  result = min(tmp, result);

  tmp = rotate(result, 4);
  result = min(tmp, result);

  tmp = rotate(result, 8);
  result = min(tmp, result);
}


/**
 * @brief Same as `rotate_min(Float, Float)`, but also returns index of smallest element.
 *
 * In the case of ties, the smallest index is returned.  
 * If min can not be determined (can't exclude), -1 is returned for index.
 */
void rotate_min(Float &input, Float &result, Int &index) {
  rotate_min(input, result);

  Float tmp     = input;
  Float minInf  = toFloat(0xff800000);  comment("Bit-value for minus infinity");

  Where (tmp > result)
    // Previously used 0 here, which was kind of stupid. 0 is a perfectly legal value.
    tmp = minInf;
  End

  Int start_elems    = 15;
  Int smallest_index = -1;
  Float tmp2;

  For (Int n = start_elems, n >= 0, n--)
    element_at(tmp, n, tmp2);
    If (tmp2 != minInf)
      smallest_index = n;
    End
  End

  index = smallest_index;
}


/**
 * @brief Return value in element `n` of `input`.
 *
 * Result is put in all the elements of the output vector.
 */
void element_at(Float const &input, Int &n, Float &result) {
  Float tmp = 0;

  Where (n == index())
    tmp = input;
  End

  rotate_sum(tmp, result);
}


/**
 * Set value of src to vector element 'n' of dst
 *
 * All other values in dst are untouched.
 *
 * @param dst integer vector to read an element from
 * @param n   index of vector element to set. Must be in range 0..15 inclusive
 * @param src integer vector to store element in
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
