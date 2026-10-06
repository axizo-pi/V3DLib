//
// Example(s) here adapted from: https://github.com/Idein/py-videocore6/blob/master/tests/test_condition_codes.py
//
// TODO perhaps translate rest of tests
//
// ============================================================================
// NOTES
// =====
//
// * This is how it appears to work:
//
//   - There are two condition flags a,b
//   - Using `push[znc]` will move current value of a to b and sets b
//   - `if[n]a` checks value of flag a, `if[n]b` checks b. The 'n' means 'not,
//     thus is true when a/b are 0.
//
// * In the test, `pushn` and `pushc` appear to do the same thing!
//
// * Condition code handling is different vor `vc4` and `v3d`:
//
//   - vc4: Each index element has associated vectors of status bits: Z,N,C
//          There is one such bit vector for the add alu and one for the mul alu
//          If field `sf (setFlags)` is set in an instruction, these are all
//          set depending on the result of the result of the instruction
//   - v3d: There are two condition bits per index element: a, b
//          There is an a/b bit for the add alu and the same set for the mul alu
//          The instruction explicitly states which condition should be tested: Z,N,C
//          The result of the condition goes into a. The previous value of a goes into b
//
///////////////////////////////////////////////////////////////////////////////
#include "support/support.h"
#include "support/check.h"    // showResult()
#include "Support/pgm.h"
#include "Support/Helpers.h"  // to_file()
#include "V3DLib.h"

using namespace V3DLib;

namespace {

using namespace V3DLib::v3d::instr;
using Instructions = V3DLib::v3d::Instructions;
using ByteCode     = V3DLib::v3d::ByteCode;
using Code         = V3DLib::Code;
using Data         = V3DLib::Data;

const int VEC_SIZE = 16;


/**
 * `cond = 'push*'` sets the conditional flag A
 */
Instructions qpu_cond_push_a() {
  Instructions ret;

  auto set_cond_push = [] (Mnemonic &instr, int index) {
    // Set a-flag with given condition
    switch (index) {
      case 0: instr.pushz(); break;  // == 0
      case 1: instr.pushn(); break;  // < 0  ? Appear to do the same thing
      case 2: instr.pushc(); break;  // < 0
    }
  };

  auto set_cond_if = [] (Mnemonic &instr, int index) {
    switch (index) {
      case 0: instr.ifa();  break;  // Test if set
      case 1: instr.ifna(); break;  // Test if not set
      case 2: instr.ifa();  break;
    }
  };

  // r2 = ptr + index*4 
  ret << eidx(r0).ldunifrf(r5)
      << mov(r2, r5)
      << shl(r0, r0, 2)
      << add(r2, r2, r0)
      << mov(r1, 4)              // r1 = 64 (offset; mov(r1, 4, 4) won't work with vc7)
      << shl(r1, r1, 4)
  ;

  for (int index = 0; index < 3; ++ index) {
    ret << eidx(r0);

    Mnemonic instr = sub(r0, r0, 10);  // r0 = index - 10
    set_cond_push(instr, index);

    ret << instr
        << mov(r0, 0);

    instr = mov(r0, SmallImm(1));
    set_cond_if(instr, index);

    ret << instr
        << mov(tmud, r0)
        << mov(tmua, r2)
        << tmuwt().add(r2, r2, r1)  // *ptr = val; ptr += 64
        << mov(r0, SmallImm(0));

    instr = nop().mov(r0, SmallImm(1));
    set_cond_if(instr, index);

    ret << instr
        << mov(tmud, r0)
        << mov(tmua, r2)
        << tmuwt().add(r2, r2, r1);
  }
  

  ret << nop().thrsw()
      << nop().thrsw()
      << nop()
      << nop()
      << nop().thrsw()
      << nop()
      << nop()
      << nop();



  return ret;
}


void reset(Int::Array &result, int val = 0) {
  for (int i = 0; i < (int) result.size(); i++) { result[i] = val; }
}


void reset(Float::Array &result, float val = 0.0) {
  for (int i = 0; i < (int) result.size(); i++) { result[i] = val; }
}


void check(Int::Array &result, int block, uint32_t *expected) {
  bool success = true;
  uint32_t n;
  std::string buf1;
  std::string buf2;

  for (n = 0; n < VEC_SIZE; ++n) {
    buf1.clear();
    for (int i = 0; i < VEC_SIZE; ++i) {
      buf1 << result[block * VEC_SIZE + i];
    }

    buf2.clear();
    for (int i = 0; i < VEC_SIZE; ++i) {
      buf2 << expected[i];
    }

    success = (result[block * VEC_SIZE + n] == (int) expected[n]);
    if (!success) break;
  }

  INFO("block " << block <<  ", index " << n);
  INFO("result  : " << buf1);
  INFO("expected: " << buf2);
  REQUIRE(success);
}


template<typename KernelType>
void run_qpu(Int::Array &result, KernelType &k, int index, uint32_t *expected) {
  INFO("Testing qpu run index: " << index);
  reset(result, -1);
  k.run();
  check(result, 0, expected);
};


/**
 * The goal is to test combined conditions in While-loops.
 */
void while_kernel(Int::Ptr result, Int count, Int y) {
  Int c   = 0;
  Int ret = 0;

  Int max = 20;

  While ((c <= count) && ((10 < y && y < max) || (count == 0)))
    ret = c;
    c++;
  End

  *result = ret;
}

}  // anon namespace


TEST_CASE("Check while condition codes [while][cond]") {
  Int::Array result(VEC_SIZE);

  auto check_result = [&result] (int max) {
    INFO("max: " << max);

    for (int i = 0; i < VEC_SIZE; ++i) {
      REQUIRE(result[i] == max);
    }
  };

  auto k = compile(while_kernel);
  to_file("while_kernel.txt", k.dump());

  int max = 123;
   k.load(&result, max, 15).run();

  check_result(max);
}


TEST_CASE("Check v3d condition codes [v3d][cond]") {

  SUBCASE("Test condition push a") {
    if (!running_on_v3d()) return;
    if (V3DLib::Platform::run_vc7()) return;  // kernel contains acc's, not supported on vc7

    const int DATA_SIZE = 16;

    Instructions k = qpu_cond_push_a();

    ByteCode bytecode = k.bytecode();
    //std::cout << Instr::mnemonics(bytecode) << std::endl;

    v3d::BufferObject heap;
    heap.alloc_bo(10*1024);  // arbitrary size, large enough

    Code code((uint32_t) bytecode.size(), heap);
    code.copyFrom(bytecode);

    Data data(6*DATA_SIZE, heap);
    for (uint32_t offset = 0; offset < data.size(); ++offset) {
      data[offset] = 0;
    }

    Data unif(1, heap);
    unif[0] = data.getAddress();

    V3DLib::v3d::Driver drv;
    drv.add_bo(heap.getHandle());
    REQUIRE(drv.execute(code, &unif));

    uint32_t pushz_if_expected[DATA_SIZE]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 };
    uint32_t pushz_ifn_expected[DATA_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 };
    uint32_t pushc_if_expected[DATA_SIZE]  = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };


    for (uint32_t n = 0; n < DATA_SIZE; ++n) {
      INFO("block 0-1, index " << n);
      REQUIRE(data[0 * DATA_SIZE + n] == pushz_if_expected[n]);
      REQUIRE(data[1 * DATA_SIZE + n] == pushz_if_expected[n]);
    }
    for (uint32_t n = 0; n < DATA_SIZE; ++n) {
      INFO("block 2-3, index " << n);
      REQUIRE(data[2 * DATA_SIZE + n] == pushz_ifn_expected[n]);
      REQUIRE(data[3 * DATA_SIZE + n] == pushz_ifn_expected[n]);
    }
    for (uint32_t n = 0; n < DATA_SIZE; ++n) {
      INFO("block 4-5, index " << n);
      REQUIRE(data[4 * DATA_SIZE + n] == pushc_if_expected[n]);
      REQUIRE(data[5 * DATA_SIZE + n] == pushc_if_expected[n]);
    }
  }
}


namespace {

void next(Int::Ptr &result, Int &r) {
  *result = r;
  result.inc();
  r = 0;
}


template<typename T, typename t>
void where_kernel_partial(T &a, t limit, Int::Ptr &result) {
  Int r = 0;

  // If limit is outside of small imm value range, it becomes a register.

  Where (a <  limit)             r = 1; End; next(result, r);
  Where (limit > a )             r = 1; End; next(result, r);
  Where (a <= limit)             r = 1; End; next(result, r);
  Where (a == limit)             r = 1; End; next(result, r);
  Where (a != limit)             r = 1; End; next(result, r);
  Where (a >  limit)             r = 1; End; next(result, r);
  Where (a >= limit)             r = 1; End; next(result, r);
  Where (!(a > (3 + limit - 8))) r = 1; End; next(result, r);
}


template<const int k_limit = 8>
void int_where_kernel(Int::Ptr result) {
  Int a = index() + (k_limit - 8);

  where_kernel_partial(a, k_limit, result);
}


void float_where_kernel(Int::Ptr result) {
  //warn << "Compiling float_where_kernel";

  Float a = toFloat(index());
  Int r = 0;

  const float limit = 8.0f;  // Stored as small imm

  where_kernel_partial(a, limit, result);
}


void andor_kernel(Int::Ptr result) {
  Int a = index();
  Int r = 0;

  Where ( a >=  4 &&  a <= 8)             r = 1; End; next(result, r);
  Where ( a <   4 ||  a >  8)             r = 1; End; next(result, r);
  Where ((a >   4 &&  a <  8) || a > 12)  r = 1; End; next(result, r); // TODO find better examples with differing res
  Where ( a >   4 && (a <  8  || a > 12)) r = 1; End; next(result, r); // BORING! Same result as previous

  Int b = index();
  Where ( a > 6 && a < 12  &&  b >  8 && b < 14)  r = 1; End; next(result, r);
  Where ( a > 6 && b < 14  &&  a < 12 && b >  8)  r = 1; End; next(result, r);
  Where ((a > 6 && a < 12) || (b >  8 && b < 14)) r = 1; End; next(result, r);

  Where ((a > 6 && a < 12) || (b >  8 && b < 14)) r = 1; Else r = 2;  End;

  *result = r;
}


void noloop_where_kernel(Int::Ptr result, Int x, Int y) {
  Int min = 10;
  Int max = 20;

  Int res = 0;  comment("Where result");

  Where (min < x && x < max && min < y && y < max )
  //Where (10 < x && x < 20 && 10 < y && y < 20 )
    res = 1;
  End

  *result = res;
}


void noloop_if_and_kernel(Int::Ptr result, Int x, Int y) {
  Int max = 20;
  Int ret = 0;

  If (10 < x && x < max && 10 < y && y< max)
    ret = 1;
  End

  *result = ret;
}


void noloop_multif_kernel(Int::Ptr result, Int x, Int y) {
  Int max = 20;
  Int ret = 0;

  If (x > 10)
    If (x < max)
      If (y > 10)
        If (y < max)
          ret = 1;
        End
      End
    End
  End

  *result = ret;
}


void andor_where_kernel(Float::Ptr result, Int width, Int height) {
  For (Int y = 0, y < height, y += 1)
    Float::Ptr p = result + y*width;  // Point p to the output row

    For (Int x = 0, x < width, x += VEC_SIZE)
      Float tmp = toFloat(1024);
      Where (y > 10 && y < 20 && x > 10 && x < 20)
        tmp = 0.0;
      End

      *p = tmp;
      p += VEC_SIZE;
    End
  End
}


void andor_if_kernel(Float::Ptr result, Int width, Int height) {
  For (Int y = 0, y < height, y += 1)
    Float::Ptr p = result + y*width;  // Point p to the output row

    For (Int x = 0, x < width, x += VEC_SIZE)
      Float tmp = toFloat(1024);
      If (y > 10 && y < 20 && x > 10 && x < 20)
        tmp = 0.0;
      End
      *p = tmp;
      p += VEC_SIZE;
    End
  End
}


void andor_multi_if_kernel(Float::Ptr result, Int width, Int height) {
  For (Int y = 0, y < height, y = y + 1)
    Float::Ptr p = result + y*width;  // Point p to the output row

    For (Int x = 0, x < width, x = x + VEC_SIZE)
      Float tmp = toFloat(1024);

      If (y > 10)
        If (y < 20)
          If (x > 10)
            If (x < 20)
              tmp = 0.0;
            End
          End
        End
      End

      *p = tmp;
      p = p + VEC_SIZE;
    End
  End
}


const int NUM_WHERE_TESTS = 8;

uint32_t where_expected[NUM_WHERE_TESTS][VEC_SIZE] = {
  /* a <     */ {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  /*   > a   */ {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
  /* a <=    */ {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
  /* a ==    */ {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
  /* a !=    */ {1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
  /* a >     */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
  /* a >=    */ {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
  /* !(a >3) */ {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const int NUM_ANDOR_TESTS = 8;

uint32_t andor_expected[NUM_ANDOR_TESTS][VEC_SIZE] = {
  /* and         */ {0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
  /* or          */ {1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1},
  /* combined    */ {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
  /* combined    */ {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1},
  /* multi_and   */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  /* multi_and   */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  /* multi_andor */ {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  /* multi_else  */ {2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 2, 2}
};


void check_result_expected(Int::Array &result, uint32_t *expected, int num_tests) {
  INFO("check_result_expected");

  for (int i = 0; i < num_tests; ++i) {
    check(result, i, &expected[i*VEC_SIZE]);
  }
}


void check_pgm(std::string const &filename) {
  std::string expected_filename = "Tests/data/where_expected.pgm";

  std::string diff_cmd = "diff " + filename + " " + expected_filename;
  INFO("diff command: " << diff_cmd);
  REQUIRE(!system(diff_cmd.c_str()));
}


}  // anon namespace


TEST_CASE("Test Where blocks [where][cond]") {
  int const SIZE      = NUM_WHERE_TESTS*VEC_SIZE;
  Int::Array result(SIZE);

  auto run_kernel = [&result] (
    RunType run_type,
    void (*kernel)(Int::Ptr result),
    bool dump_kernel = false
  ) {
    static int dump_count;

    reset(result);

    BaseSettings settings;
    settings.run_type = run_type;

    auto k = compile(kernel, settings);

    if (dump_kernel) {
      std::string filename = "run_kernel_";
      filename << dump_count << ".txt";
      to_file(filename, k.dump());

      dump_count++;
    }

    k.load(&result).run();
    check_result_expected(result, (uint32_t *) where_expected, NUM_WHERE_TESTS);
  };

  SUBCASE("Testing int_where_kernel k_limit=8") {
    run_kernel(QPU        , int_where_kernel, true);
    run_kernel(Emulator   , int_where_kernel);
    run_kernel(Interpreter, int_where_kernel);
  }

  SUBCASE("Testing int_where_kernel k_limit=20") {
    run_kernel(QPU        , int_where_kernel<20>);
    run_kernel(Emulator   , int_where_kernel<20>);
    run_kernel(Interpreter, int_where_kernel<20>);
  }

  SUBCASE("Testing float_where_kernel") {
    run_kernel(QPU        , float_where_kernel);
    run_kernel(Emulator   , float_where_kernel);
    run_kernel(Interpreter, float_where_kernel);
  }
}


/**
 * This is meant as a precursor for the following test,
 * to ensure that the contents of the double loops work as expected
 */
TEST_CASE("Test if/where without loop [noloop][cond]") {
  make_test_dir();
  uint32_t zeroes[VEC_SIZE]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint32_t ones[VEC_SIZE]  = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  Int::Array result(VEC_SIZE);

  const int NUM_PARAMS = 9;

  struct param_struct {
    param_struct(int in_x, int in_y, bool in_ones) : x(in_x), y(in_y), ones(in_ones) {}

    int x;
    int y;
    bool ones;
  };


  // All combinations of inside/outside ranges of x and y together
  param_struct params[NUM_PARAMS] = {
    {   0,  0, false},
    {  13,  0, false},
    { 123,  0, false},
    {   9, 13, false},
    {  12, 15,  true},
    {  13, 22, false},
    { 100,  6, false},
    { 120, 16, false},
    { 100, 26, false}
  };

  auto check_params = [&result, &params, NUM_PARAMS, &ones, &zeroes] (BaseKernel &k) {
    for (int i = 0; i < NUM_PARAMS; ++i) {
      auto &p = params[i];

      k.load(&result, p.x,  p.y);
      run_qpu(result, k, i, p.ones?ones:zeroes);
    }
  };


  SUBCASE("Testing noloop_where_kernel") {
    auto k = compile(noloop_where_kernel);

    check_params(k);
  }

  SUBCASE("Testing noloop_if_and_kernel") {
    auto k = compile(noloop_if_and_kernel);
    check_params(k);
  }

  SUBCASE("Testing noloop_multif_kernel") {
    auto k = compile(noloop_multif_kernel);
    check_params(k);
  }
}


TEST_CASE("Test multiple and/or [andor][cond]") {
  make_test_dir();

  int const width  = 48;
  int const height = 32;

  auto check_output_pgm = [width, height] (Float::Array &result, char const *label) {
    std::string filename = test_path();
    filename <<  "/andor_" << label << "_output.pgm";
    //warn << "filename: " << filename;

    output_pgm_file(result, width, height, 255, filename.c_str());
    check_pgm(filename.c_str());
  };


  SUBCASE("Test Where blocks with and/or") {
    const int NUM_TESTS = 9;
    Int::Array result(NUM_TESTS*VEC_SIZE);

    auto k = compile(andor_kernel);
    k.load(&result).run();
    check_result_expected(result, (uint32_t *) andor_expected, NUM_ANDOR_TESTS);
  }


  SUBCASE("Test andor_where_kernel") {
    Float::Array result(width*height);

    auto k1 = compile(andor_where_kernel);
    k1.load(&result, width, height).run();
    check_output_pgm(result, "where_qpu");
  }


  SUBCASE("Test andor_if_kernel") {
    Float::Array result(width*height);

    reset(result);
    auto k2 = compile(andor_if_kernel);
    k2.load(&result, width, height).run();
    check_output_pgm(result, "if_qpu");
  }


  SUBCASE("Test andor_multi_if_kernel") {
    Float::Array result(width*height);

    auto k3 = compile(andor_multi_if_kernel);
    reset(result);
    k3.load(&result, width, height).run();
    check_output_pgm(result, "multi_if_qpu");
  }
}
