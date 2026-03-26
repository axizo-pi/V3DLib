#include "V3DLib.h"
#include "Support/Settings.h"
#include "LibSettings.h"
#include "Support/Helpers.h"  // to_file()

using namespace V3DLib;

V3DLib::Settings settings;

void kernel(Int::Ptr p) {
  Int x = 0, y; // 0 assignment required (20260326 used to work, unclear why not now)
                // Setting y = 0 works as well
  gather(p);
  gather(p + 16);
  receive(x);
  receive(y);

 *p = x + y;
}


int main(int argc, const char *argv[]) {
  //LibSettings::dump_line_numbers(false);

  settings.init(argc, argv);

  auto k = compile(kernel, settings);             // Construct kernel
  //to_file("reqrecv_2.txt", k.dump());

  Int::Array array(2*16);                         // Alloc and init array shared between ARM and GPU
  for (int i = 0; i < (int) array.size(); i++) {
    array[i] = i;
  }

  k.load(&array);                                 // Invoke the kernel
  k.run();

  for (int i = 0; i < (int) array.size()/2; i++)  // Display the result
    printf("%i: %i\n", i, array[i]);
  
  return 0;
}
