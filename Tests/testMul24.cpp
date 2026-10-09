/**
 * /file
 *
 * Sincere and determined attempt to understand how vc4 operation `mul24` works.
 * At the very least, interpreter (and emulator) output should be the same.
 */
#include "doctest.h"
#include "V3DLib.h"
#include "support/support.h"
#include "Support/Helpers.h"  // to_file()
#include <iomanip>

using namespace V3DLib;

namespace {


const int N = 10;  // Number of results returned


void mul24_kernel(Int::Ptr res) {
  // Mult of 2 int constants translate directly to an immdediate.
  // There is no mul24. Output is exact.
  nop(1); sub_header("-15*16");
  *res = -15*16;           res.inc();
  nop(1); sub_header("15*123");
  *res = 15*123;           res.inc();

  //
  // For all the following operations mul24 is used
  //

  Int a = 16;

  // -1 small imm
  nop(1); sub_header("-1*a");
  *res = -1*a;             res.inc();

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

  // result -1,0,+1 within range
  nop(1); sub_header("0x1000^2");
  Int val = 0x1000;
  *res = val*val;         res.inc();

  nop(1); sub_header("(0xffff)^2");
  val = 0x10000;
  *res = (val - 1)*(val - 1); res.inc();

  // result is zero!
  nop(1); sub_header("(0x10000)^2");
  *res = val*val;         res.inc();

  nop(1); sub_header("(0x10001)^2");
  *res = (val + 1)*(val + 1); res.inc();
}


/**
 * @brief Show first values per row
 */
std::string show_first(int *p, int num) {
  std::stringstream ret;
  ret << "<";

  for (int i = 0; i < num; ++i) {
    int val = p[i*16];
    ret << val << " (0x" << std::hex << val << std::dec << "), ";
  }

  ret << ">\n";
  return ret.str();
}


}  // anon namespace

TEST_CASE("Test Mul24 values [mul24]") {
  if (!Platform::run_vc4()) {
    warn << "Unit test [mul24] only for vc4";
    return;
  }


  SUBCASE("Run QPU") {
    Int::Array result(16*N);

    auto k = compile(mul24_kernel);
    to_file("mul24_kernel.txt", k.dump());
    k.load(&result).run();

    //warn << dump_array(result, 16);
    warn << "First: " << show_first(result.ptr(), N);
  }


  SUBCASE("Run Interpreter") {
    Int::Array result(16*N);

    auto k = compile(mul24_kernel);
    k.load(&result).interpret();

    warn << "First: " << show_first(result.ptr(), N);
  }
}
