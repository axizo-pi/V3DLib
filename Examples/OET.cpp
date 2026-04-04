/**
 * OET - Odd-even transposition sorter for 32 integers.
 *
 * Reference: https://en.wikipedia.org/wiki/Odd%E2%80%93even_sort
 */
#include "V3DLib.h"
#include "Support/Settings.h"
#include "LibSettings.h"
#include "Support/Helpers.h"  // to_file()

using namespace V3DLib;

V3DLib::Settings settings;

/**
 * @brief Kernel for Odd/even transposition sorter.
 *
 * ============================
 *
 * Note 1
 * ------
 *
 * 20260326 Interim issue vc4 with TMU load:  
 * Either p or p2 outputs max value for the range, not all values.
 * DMA load works fine.
 */
void kernel(Int::Ptr p) {
	Int::Ptr p2 = p;  // See Note 1
	p2.inc();

  Int evens = *p;
  Int odds  = *p2;

  For (Int count = 0, count < 16, count++)
    Int evens2 = min(evens, odds);
    Int odds2  = max(evens, odds);

    Int evens3 = rotate(evens2, 15);
    Int odds3  = odds2;
    Where (index() != 15)
      odds2 = min(evens3, odds3);
    End

    Where (index() != 0)
      evens2 = rotate(max(evens3, odds3), 1);
    End

    evens = evens2;
    odds  = odds2;
  End

  *p  = evens;
  *p2 = odds;
}


int main(int argc, const char *argv[]) {
  //LibSettings::dump_line_numbers(false);
  LibSettings::use_tmu_for_load(false);  // Use DMA

  settings.init(argc, argv);

  auto k = compile(kernel, settings);             // Construct kernel
  //to_file("oet_2.txt", k.dump());

  Int::Array a(32);                               // Allocate and initialise array shared between ARM and GPU
  for (int i = 0; i < (int) a.size(); i++)
    a[i] = 100 - i;

  k.load(&a).run();                               // Load the uniforms and invoke the kernel

  for (int i = 0; i < (int) a.size(); i++)        // Display the result
    printf("%i: %i\n", i, (i & 1) ? a[16+(i>>1)] : a[i>>1]);
  
  return 0;
}
