/* =================================================================

\file

Examination of loops limits.

There appears to be a limit on the number of loop iterations you can do within a kernel.
This test programs attempts to determine the upper limits.


A 'failed run' is defined as a run with no output, e.g.:

    Counter:  0.513213s
    result: (
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    )

On a successful run, all result values will be non-zero.

**NOTE:** Running Counter often enough appears to slow down vc7 significantly.
          Eventually, vc7 seems to recover.

Magic number `7` in following is simply the number of runs whose results all fit on the screen.
Max int: 2,147,483,647

vc7
----------------

## No y_loop

- Fair assumptions:
  int x_count >= 5900000; always fails
  int x_count <= 5000000; always succeeds

- Tested values:
  int x_count = 6050000; 0/7
  int x_count = 6010000;
  int x_count = 6000000; 0/7
  int x_count = 5900000; 0/7
  int x_count = 5800000;
  int x_count = 5700000;
  int x_count = 5600000; 5/7
  int x_count = 5599000; 4/7
  int x_count = 5590000;
  int x_count = 5580000; 5/7
  int x_count = 5550000; 4/7
  int x_count = 5500000; 7/7, 6/7
  int x_count = 5000000; 7/7, 10/10
  int x_count = 4000000;
  int x_count = 2000000;
  int x_count = 1000000;
  int x_count = 800000; 7/7
  int x_count = 750000;
  int x_count = 625000; 7/7

## y_loop = 1

  int x_count = 5000000; 0/7
  int x_count = 3000000; 5/7
  int x_count = 2000000; 7/7
  int x_count = 1000000; 7/7

## y_loop = 10

  int x_count = 2000000; 0/7
  int x_count = 1000000; 0/7
  int x_count = 950000; 2/7
  int x_count = 900000; 7/7
  int x_count = 800000; 7/7
  int x_count = 600000; 7/7
  int x_count = 200000; 7/7

## y_loop = 100

  int x_count = 950000; 0/7
  int x_count = 500000; 0/7
  int x_count = 300000; 0/7
  int x_count = 200000; 0/7
  int x_count = 150000; 0/7
  int x_count = 125000; 0/7
  int x_count = 120000; 4/7
  int x_count = 110000; 7/7
  int x_count = 100000; 7/7, 10/10


## Previous comments

  // vc7 testing: 
  //int x_count = 8000000;    // Always fails
  //int x_count = 3529475;  // Fails intermittently 
  //int x_count = 3529400;  // Fails intermittently
  //int x_count = 3529000;  // idem
  //int x_count = 3520000;  // idem
  //int x_count = 3500000;  // idem
  //int x_count = 3400000;    // Worked 10x in a row; between values not tested

vc6
----------------

## No y_loop

  int x_count = 10000000; 0/7
  int x_count = 8000000; 0/7
  int x_count = 7000000; 0/7
  int x_count = 6500000; 0/7
  int x_count = 6300000; 0/7
  int x_count = 6100000; 2/7
  int x_count = 6000000; 7/7, 10,10
  int x_count = 5000000; 7/7
  int x_count = 1000000; 7/7
  int x_count = 120000; 7/7

## y_loop = 1

  int x_count = 6100000; 0/7
  int x_count = 4000000; 0/7
  int x_count = 3500000; 0/7
  int x_count = 3350000; 0/7
  int x_count = 3300000; 2/7
  int x_count = 3250000; 7/7
  int x_count = 3000000; 7/7
  int x_count = 2000000; 7/7
  int x_count = 1000000; 7/7
  int x_count = 600000; 7/7

## y_loop = 10

  int x_count = 3300000; 0/7
  int x_count = 1000000; 2/7
  int x_count = 990000; 6/7 (first failed)
  int x_count = 980000; 7/7
  int x_count = 950000; 7/7
  int x_count = 900000; 7/7
  int x_count = 700000; 7/7
  int x_count = 100000; 7/7

## y_loop = 100

  int x_count = 990000; 0/7
  int x_count = 500000; 0/7
  int x_count = 300000; 0/7
  int x_count = 200000; 0/7
  int x_count = 150000; 0/7
  int x_count = 135000; 0/7
  int x_count = 127500; 0/7
  int x_count = 124000; 4/7
  int x_count = 122000; 7/7
  int x_count = 120000; 7/7
  int x_count = 100000; 7/7


## Previous comments

  // vc6 testing:
  // y=100 times out
  // y=50 ends without a returned reason but timer > 10
  // y=30 hangs but works after reset
  //k.load(x_count, 20, &result);

   ================================================================= */
#include <string>
#include <V3DLib.h>
#include <CmdParameters.h>
#include "Support/Timer.h"
#include "Support/Settings.h"
#include "Support/pgm.h"
#include "vc4/RegisterMap.h"
#include "Source/Complex.h"

using namespace V3DLib;
using std::string;

const int Do_YLoop = true;

void kernel(Int x_count, Int y_count, Int::Ptr result) {
  Int count   = 0;
  Int count_y = -1;

  For (Int x = 0, x < x_count, x++)
		if (Do_YLoop) {
      For (Int y = 0, y < y_count, y++)
			  count_y++;
      End
		}

    count += 1;

    Int::Ptr r2 = result + 32;
    *r2  = count_y;
  End

  Int::Ptr r2 = result + 16;
  *r2  = 10;

  *result = count;   // vc7 This never returns a value? vc6 works
}


int main(int argc, const char *argv[]) {
  Settings settings;
  settings.init(argc, argv);

  auto k = compile(kernel, settings);
  k.setNumQPUs(settings.num_qpus);

  int x_count = 124000;
  int y_count = 100;

  Int::Array result(3*16);

  k.load(x_count, y_count, &result);

  Timer timer("counter");
  k.run();

	if (!settings.silent) {
		std::cout << "Counter: " << timer.dump() << "\n";
		std::cout << "result: " << result.dump() << "\n";
	}


  return 0;
}
