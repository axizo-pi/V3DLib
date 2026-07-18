#include <V3DLib.h>
#include "support/support.h"
#include "support/check.h"

using namespace V3DLib;

namespace {

void clz_kernel(Int::Ptr result, Int::Ptr input, Int Blocks) {
  For (Int i = 0, i < Blocks, i++)
    Int val = *input;
    *result = clz(val);

    input.inc();
    result.inc();
  End
}

} // anon namespace


TEST_CASE("Test LZ - Count Leading Zeroes [clz]") {
  const int Blocks = 3;

  std::vector<unsigned> values = {
    0x00000000,
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000,

    // Rest of the values are fillers,
    // do something random that mostly return zero
    0x80020200, 0x8f001000, 0x8e000000, 0x80000430,
    0x80123000, 0x8fff0000, 0x80000000, 0x80111100,
    0x80000001, 0x80000002, 0x80100003, 0xf0111104,
    0x7f001001, 0x3f400002, 0x101e0003
  };

  std::vector<int> values_int(16*Blocks);

  // Convert values bitwise to int
  for (int i = 0; i < (int) values.size(); ++i) {
    values_int[i] = *((int *) &values[i]);
  }

  Int::Array input(16*Blocks);
  input.copyFrom(values_int);
  //warn << "input: " << dump_array(input);

  std::vector<int> expected = {
    32,
    31, 30, 29, 28, 27, 26, 25, 24,
    23, 22, 21, 20, 19, 18, 17, 16,
    15, 14, 13, 12, 11, 10,  9,  8,
     7,  6,  5,  4,  3,  2,  1,  0,
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  1,  2,  3
  };

  Int::Array result(16*Blocks);

  auto k = compile(clz_kernel);
  k.load(&result, &input, Blocks).run();
  //warn << "result: " << dump_array(result);

  check_vector(result, 0, expected);
}
