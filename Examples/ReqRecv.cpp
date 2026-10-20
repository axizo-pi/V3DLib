#include "V3DLib.h"
#include "Support/Settings.h"
#include "LibSettings.h"
#include "Support/Helpers.h"  // to_file()

using namespace V3DLib;

V3DLib::Settings settings;

/**
 *
 * ============================
 * Notes
 * -----
 *
 * 1. 20260326 x = 0 on vc4 with TMU load.  
 *    DMA load works fine.
 *
 *    0 assignment required; setting `y = 0` works as well.  
 *    Used to work, unclear why not now.
 *
 *    It makes no sense because the acc used gets reset quite quickly after the 0 assignment.  
 *    The only association I see is that this acc is present in a mul NOP, which is in between.
 *    But this should do nothing.
 */
void kernel(Int::Ptr p) {
  Int x = 0, y; // See Note 1

  gather(p);
  gather(p + 16);
  receive(x);
  receive(y);

  *p = x + y;
}


int main(int argc, const char *argv[]) {
  settings.init(argc, argv);

  auto k = compile(kernel, settings);               // Construct kernel

  Int::Array array(2*16);                           // Alloc and init array shared between ARM and GPU
  for (int i = 0; i < (int) array.size(); i++) {
    array[i] = i;
  }

  k.load(&array).run();                             // Invoke the kernel

  for (int i = 0; i < (int) array.size()/2; i++) {  // Display the result
    std::cout << i << ": " << array[i] << "\n";
  }
  
  return 0;
}
