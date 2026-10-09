#include "doctest.h"
#include "V3DLib.h"
#include "support/support.h"
#include "Support/Helpers.h"  // to_file()

using namespace V3DLib;

namespace {

void mul24_kernel(Int::Ptr res) {
  *res =  123;
  res.inc();

  *res = -1*16;              sub_header("-1*16");
  res.inc();

  Int a = 16;
  Int b = -1 * a;
  // vc4: b = 268435440
  *res = b;                  sub_header("b");
  res.inc();

  //Int val = 0xfffff;
  Int val = 0xfff;
  *res = (val - 1)*(val - 1);
  res.inc();
}


}  // anon namespace

TEST_CASE("Test Mul24 values [mul24]") {
  if (!Platform::run_vc4()) {
    warn << "Unit test [mul24] only for vc4";
    return;
  }
  warn << "Test Mul24 values [mul24]";

  int N = 4;  // Number of results returned

  SUBCASE("Run QPU") {
    Int::Array result(16*N);

    auto k = compile(mul24_kernel);
    to_file("mul24_kernel.txt", k.dump());
    k.load(&result).run();

    warn << dump_array(result, 16);
  }


  SUBCASE("Run Interpreter") {
    Int::Array result(16*N);

    auto k = compile(mul24_kernel);
    k.load(&result).interpret();

    warn << dump_array(result, 16);
  }
}
