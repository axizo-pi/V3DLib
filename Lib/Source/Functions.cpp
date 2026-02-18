/******************************************************************************
 * Function Library for functions at the source language level
 *
 * These are not actual functions but generate inlined code.
 * In the kernel code, though, they look like function calls.
 *
 ******************************************************************************/
#include "Functions.h"
#include <iostream>
#include <cmath>
#include "Support/Platform.h"
#include "global/log.h"
#include "StmtStack.h"
#include "Lang.h"
#include "LibSettings.h"
#include "vc4/Functions.h"
#include "vc4/RegisterMap.h"

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

    Int tmp = 4;   comment("Load INF");  // Important that comment is AFTER first statement
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
 
  comment("End long integer division");  // For some reason, phantom instances of this comment can pop up elsewhere
                                         // in code dumps. Not bothering with correcting this.
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

    //return res;
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
  // setting to true in param overrides lib setting
  extra_precision |= LibSettings::use_high_precision_sincos();

  double x = x_in;

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
 */
FloatExpr cos(FloatExpr x_in, bool extra_precision) {
  // setting to true in param overrides lib setting
  extra_precision |= LibSettings::use_high_precision_sincos();

  Float x = x_in;

  x -= 0.25f + functions::ffloor(x + 0.25f);  comment("Start cosine");
  x *= 16.0f * (fabs(x) - 0.5f);

  if (extra_precision) {
    //Log::warn << "doing extra precision";
    x += 0.225f * x * (fabs(x) - 1.0f);
  }

  return x;
}


FloatExpr sin(FloatExpr x_in, bool extra_precision) {
  return cos(0.25f - x_in, extra_precision);
}


/**
 * Calculate sine for v3d using  hardware
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


namespace {

/**
 * Let QPUs wait for each other.
 *
 * This is a busy wait!
 * It is also an **ugly hack** for something which should be supported by hardware.  
 * The only reason it works is that the `signal` value is retained in the L2 cache.
 *
 * On `v3d` this is superseded by `barrier()`, which works fine.  
 * On `vc4`, the same should be possible by use of semaphores, but that doesn't work right now (20260207).
 *
 * In any case, last I looked it worked on `vc4` also.
 */
MAYBE_UNUSED void sync_qpus(Int::Ptr signal) {
  If (numQPUs() != 1) // Don't bother syncing if only one qpu
    *(signal - index() + me()) = 1;

    header("Start QPU sync");

    If (me() == 0)
      Int expected = 0;   comment("QPU 0: Wait till all signals are set");
      Where (index() < numQPUs())
        expected = 1;
      End 

      Int tmp = *signal;
      While (expected != tmp)
        tmp = *signal;
      End

      *signal = 0;        comment("QPU 0 done waiting, let other qpus continue");
    Else
      Int tmp = *signal;  comment("Other QPUs: Wait till all signals are cleared");

      While (0 != tmp)
        tmp = *signal;
      End
    End
  End
}


/**
 * **TODO*: not implemented in emulator, consider.
 */
void mutex_acquire() {
  assert(Platform::compiling_for_vc4());

  Expr::Ptr dummy = mkVar(Var(DUMMY));
  Expr::Ptr mutex = mkVar(Var(MUTEX_ACQUIRE));  // Read A/B

  Stmt::Ptr ptr = Stmt::create_assign(dummy, mutex);
  stmtStack().push(ptr);
}


/**
 * **TODO*: not implemented in emulator, consider.
 *
 * Can't use DUMMY as src var. Fails on Taget translation.
 */
void mutex_release() {
  assert(Platform::compiling_for_vc4());

  Expr::Ptr mutex = mkVar(Var(MUTEX_RELEASE));  // Write A/B

  Stmt::Ptr ptr = Stmt::create_assign(mutex, IntExpr(0).expr() /*dummy*/);
  stmtStack().push(ptr);
}


/**
 * @brief barrier implementation for `vc4`
 *
 * You can't assume that `QPU 0` enters this routine first.
 *
 * **Pre**: signal vectors in main memory are set to zero.
 *
 * ---------------------------------------------
 *
 * Notes
 * -----
 *
 * - This code is for `vc4`. `v3d` has a `barrier` operation, which
 *   is much more convenient.
 *
 * - This code requires that:
 *   * _either_ the L2 cache is disabled
 *   * _or_ DMA reads are used
 */
void vc4_barrier(Int::Ptr signal) {
  assert(Platform::compiling_for_vc4());
  Log::assertq(!LibSettings::use_tmu_for_load() || !RegisterMap::L2CacheEnabled(),
    "vc4_barrier(): For this call to work, either use DMA for read, "
    "or disable the L2 cache."
  );

  auto check_signals = [&signal] (Int &all_signals_set) {
    Int dummy = 1;   comment("Check all signals");  // TODO replace with Source NOP
    all_signals_set = 1;

    For (Int i = 0, i < numQPUs(), i++)
      Int tmp = *(signal + 16*i);

      // TODO: implement operator&& for Int's
      If (tmp == 0) 
        all_signals_set = 0;
      End
    End
  };


  // 'I' refers to the QPU currently within this code
  // 'We' refers to all QPU's participating

  // Following is a placeholder for setting the header.
  // Better would be to add NOP at the source level.
  // TODO: implement Source NOP. Alternatively, add NOP for header() if no stmt present.
  Int dummy = 1;   header("vc4 barrier");

  If (numQPUs() != 1)                     // Don't bother if only one QPU
    //
    // Grab the mutex, setting the signals beforehand
    //
    *(signal + 16*me()) = 1;              // Signal that I am here
    
    Int::Ptr leader_ptr = (signal + 16*(numQPUs() + 1));

    // Strong assumption: only one QPU can grab the mutex at any time
    mutex_acquire();  comment("mutex_acquire");
    //Log::warn << "mutex_acquire: " << stmtStack().last_stmt()->dump();

    Int leader_signal = *leader_ptr;

    If (leader_signal == 0)
      // I am the barrier leader
      *leader_ptr = 1;

      // Wait for all signals to be set
      Int all_signals_set;
      check_signals(all_signals_set);
      While (all_signals_set == 0)
        check_signals(all_signals_set);
      End

      // We're all here, release the mutex
      mutex_release();  comment("mutex_release");

      //
      // Reset all signals in main memory for next round
      //
      // There _may_ be a race condition here, since other QPU's
      // are released before the leader signal is reset.
      // 
      For (Int i = 0, i < (numQPUs() + 1), i++) 
        *signal = 0;      comment("Release all signals in loop");
        signal.inc();
      End
    Else
      // I am not the barrier leader.
      // Wait till the mutex is acquired and release for the rest.
      mutex_release();  comment("mutex_release");
    End
  End
}

} // anon namespace


/**
 * Has no inputs, only an output, which is always magic reg SYNCB.
 *
 * `barrier` is v3d-specific. vc4 will need a different implementation,
 * most likely with semaphores.
 */
void barrier(Int::Ptr &signal) {
  if (Platform::compiling_for_vc4()) {
    Log::warn << "barrier vc4";
    // vc4 - Stmt::BARRIER will not be passed on
    vc4_barrier(signal);
    //sync_qpus(signal);
  } else {
    // v3d
    stmtStack().push(Stmt::create(Stmt::BARRIER));
  }
}

}  // namespace V3DLib
