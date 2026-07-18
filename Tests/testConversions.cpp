#include <V3DLib.h>
#include "support/support.h"
#include "support/check.h"

using namespace V3DLib;

namespace {

void signed_to_float_kernel(Float::Ptr result, Int::Ptr input, Int Blocks) {
  For (Int i = 0, i < Blocks, i++)
    Int   n = *input;
    Float r = toFloat(n);
    *result = r;

    input.inc();
    result.inc();
  End
}


void unsigned_to_float_kernel(Float::Ptr result, Int::Ptr input, Int Blocks) {
  For (Int i = 0, i < Blocks, i++)
    Int   n = *input;
    Float r = UnsignedtoFloat(n);
    *result = r;

    input.inc();
    result.inc();
  End
}

const int Blocks = 4;

std::vector<int> values = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 , 15,
  0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14 , -15,
  22951, 82873, 83242, 90738, 91688, 23135, 45929, 54547, 28837, 74328, 80697, 48614, 82437, 49339, 3386, 70684,
  -22951, -82873, -83242, -90738, -91688, -23135, -45929, -54547, -28837, -74328, -80697, -48614, -82437, -49339, -3386, -70684
};

} // anon namespace


TEST_CASE("Test conversion of signed to float [convert]") {
  Int::Array input(16*Blocks);
  input.copyFrom(values);
  //warn << "input: " << dump_array(input);

  Float::Array result(16*Blocks);

  auto k = compile(signed_to_float_kernel);
  k.load(&result, &input, Blocks).run();
  //warn << "result: " << dump_array(result);

  // Float result is compared to int values
  check_vector(result, 0, values);
}


TEST_CASE("Test conversion of unsigned to float [convert]") {
  Int::Array input(16*Blocks);
  input.copyFrom(values);
  //warn << "input: " << dump_array(input);

  std::vector<unsigned> expected(16*Blocks);
  REQUIRE(expected.size() == values.size());

  for (int i = 0; i < (int) values.size(); ++i) {
    expected[i] = *((unsigned *) &values[i]);
  }
  //warn << "expected: " << dump_array(expected);

  Float::Array result(16*Blocks);

  auto k = compile(unsigned_to_float_kernel);
  k.load(&result, &input, Blocks).run();
  //warn << "result: " << dump_array(result);

  // Float result is compared to unsigned values
  check_vector(result, 0, expected);
}
