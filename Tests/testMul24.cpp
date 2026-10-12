/**
 * /file
 *
 * Sincere and determined attempt to understand how vc4 operation `mul24` works.
 * At the very least, interpreter (and emulator) output should be the same.
 *
 * ==============================================
 * Conclusions
 * -----------
 *
 * 1. Mult of 2 int constants translate directly to an immediate.
 *    There is no mul24. Output is exact.
 *
 * 2. Only the bottom 24 bits are taken into account, irrespective of sign.
 *    The top 8 bits are totally ignored.
 *
 *    The multiplication is basically unsigned.
 *    The result is 32-bit, so it can accidentaly become negative.
 *
 *    **NOTE:** The significand of a 32-bit IEEE floating point is also 24 bits.
 *              This can't be a coincidence.
 */
#include "doctest.h"
#include "V3DLib.h"
#include "support/support.h"
#include "support/check.h"
#include "Support/Helpers.h"  // to_file()
#include <iomanip>

using namespace V3DLib;

namespace {

std::string show_val(int val) {

  auto hex = [] (int val) -> std::string {
    std::stringstream ret;
    ret << "0x" << std::hex << val << std::dec;
    return ret.str();
  };

  std::string ret;
  ret << pad(val)          << " "
      << pad(hex(val), 10) << " ";


  // Display what the value would be if negativity was extended/
  // This is only done if the top bit of a 24-bit value is set.
  if (val & 0x800000) {
    int min_val = val | 0xf0000000;
    ret << pad(min_val);
  } else {
    ret << pad("", 10);
  }

  return ret;
}


std::string val_header() {
  std::string ret;
  ret << "\n"
      << "   Value      Hex        Neg\n"
      << "  ========== ========== ==========\n";

  return ret;
}


/**
 * @brief Show first values per row.
 *
 * Shows hex values as well.
 */
MAYBE_UNUSED std::string show_first(int *p, int num) {
  std::string ret;
  ret << val_header();

  for (int i = 0; i < num; ++i) {
    int val = p[i*16];
    ret << "  " << show_val(val) << "\n";
  }

  ret << "\n";
  return ret;
}


/**
 * @brief show values of array, along with hex values.
 */
MAYBE_UNUSED std::string show_arr(int *p, int num = 1) {
  assert(num ==1);  // Deal with this when it happens
  std::string ret;
  ret << val_header();

  for (int i = 0; i < 16; ++i) {
    int val = p[i];
    ret << "  " << show_val(val) << "\n";
  }

  ret << "\n";
  return ret;
}


/////////////////////////////////////////////////////////////////
// Kernels for unit test
/////////////////////////////////////////////////////////////////

const int N = 9;  // Number of results returned

void mul24_imm_kernel(Int::Ptr res) {
  nop(1); sub_header("Two int constants");     // Conclusion 1.
  *res = -15* 16;          res.inc();
  *res =  15*123;          res.inc();

  //
  // For all the following operations mul24 is used
  //

  Int a = 16;

  nop(1); sub_header("<small imm>*a");
  *res = -15*a;            res.inc();
  *res =  -5*a;            res.inc();
  *res =  -1*a;            res.inc();
  *res =  15*a;            res.inc();

  // -31 large imm
  nop(1); sub_header("-31*a");
  *res = -31*a;            res.inc();

  // 31 large imm
  nop(1); sub_header("31*a");
  *res = 31*a;            res.inc();


  // vc4: b = 268435440
  nop(1); sub_header("b");
  Int b = -1 * a;
  *res = b;               res.inc();
}


void mul24_kernel(Int::Ptr p_a, Int::Ptr p_b, Int::Ptr res) {
  Int a = *p_a;
  Int b = *p_b;

  nop(1); sub_header("Do mul");
  Int ret = a*b;

  nop(1); sub_header("Return value");
  *res = ret;
}

}  // anon namespace


TEST_CASE("Test Mul24 values immediates [mul24]") {
  if (!Platform::run_vc4()) {
    warn << "Unit test [mul24] only for vc4";
    return;
  }

  Int::Array result_qpu(16*N);
  Int::Array result_int(16*N);

  auto k = compile(mul24_imm_kernel);
  //to_file("mul24_imm_kernel.txt", k.dump());

  {
    INFO("Run QPU");
    k.load(&result_qpu).run();
    //warn << "Imm QPU: " << show_first(result_qpu.ptr(), N);
  }

  {
    INFO("Run Interpreter");
    k.load(&result_int).interpret();
    //warn << "Imm Interpreter: " << show_first(result_int.ptr(), N);
  }

  // Output of QPU and interpreter should be the same.
  check_vector_ptr(result_qpu.ptr(), result_int.ptr(), result_qpu.size());
}


TEST_CASE("Test Mul24 values [mul24]") {
  if (!Platform::run_vc4()) {
    warn << "Unit test [mul24] only for vc4";
    return;
  }

  // The input vectors are split to align a and b values within the view margin,
  // and also to add aligned comments.
                                                  // fail  , fail
  std::vector<int> input_a  = {  1, -1, -31, 0xffff, 0xff00, 0xffff, 0x10000, 0x10001, 0x10000};
  std::vector<int> input_b  = { 16, 16,  16, 100   , 0xff00, 0xffff, 0x10000, 0x10001, 0x10000};

                                                              // 0    ,  0    ,  0
  std::vector<int> input_a2 = {  0xffffff, 0xffffff, 0x7fffffff, 65536, -65536, -65536, 0x100};
  std::vector<int> input_b2 = {  2       , -2      , 1         , 65536,  65536, -65536, 0x100};

  input_a.insert(input_a.end(), input_a2.begin(), input_a2.end());
  input_b.insert(input_b.end(), input_b2.begin(), input_b2.end());

  Int::Array a(16);
  Int::Array b(16);
  Int::Array result_qpu(16);
  Int::Array result_int(16);

  a.copyFrom(input_a);
  b.copyFrom(input_b);

  auto k = compile(mul24_kernel);
  //to_file("mul24_kernel.txt", k.dump());

  {
    INFO("Run QPU");
    k.load(&a, &b, &result_qpu).run();
    //warn << "Val QPU: " << show_arr(result_qpu.ptr());
  }

  {
    INFO("Run Interpreter");
    k.load(&a, &b, &result_int).interpret();
    //warn << "Val Interpreter: " << show_arr(result_int.ptr());
  }

  // Output of QPU and interpreter should be the same.
  check_vector_ptr(result_qpu.ptr(), result_int.ptr(), result_qpu.size());
}
