#include "V3DLib.h"
#include "Support/Settings.h"

using namespace V3DLib;

V3DLib::Settings settings;


void kernel_1(Int offset, Int::Ptr p) {
  p += offset*16;
	*p = 1;
}


void kernel_2(Int offset, Int::Ptr p) {
  p += offset*16;
	*p = 2;
}


void kernel_3(Int offset, Int::Ptr p) {
  p += offset*16;
	*p = 4;
}


/**
 * Extra param to mess with invoke()
 */
void kernel_4(Int offset, Int mult, Int::Ptr p) {
  p += offset*16;
	*p = mult*8;
}


void kernel_5(Int offset, Int::Ptr p) {
  p += offset*16;
	*p = 16;
}


int main(int argc, const char *argv[]) {
  int num_kernels = 5;

  settings.init(argc, argv);

  auto k_1 = compile(kernel_1, settings);              // Construct the kernels
  auto k_2 = compile(kernel_2, settings);
  auto k_3 = compile(kernel_3, settings);
  auto k_4 = compile(kernel_4, settings);
  auto k_5 = compile(kernel_5, settings);

  Int::Array array(num_kernels*16);                   // Allocate and initialise the array shared between ARM and GPU
  array.fill(0);

  k_1.load(0, &array);
  k_2.load(1, &array);
  k_3.load(2, &array);
  k_4.load(3, 3, &array);
  k_5.load(4, &array);

  Invoke::schedule(k_1);
  Invoke::schedule(k_2);
  Invoke::schedule(k_3);
  Invoke::schedule(k_4);
  Invoke::schedule(k_5);
  Invoke::run();

  for (int k = 0; k < num_kernels; k++) {              // Pedantic: check if all values from a given QPU are  the same 
    int start = k*16;
    for (int i = 1; i < 16; i++) {  
      assert(array[start] == array[start + i]);
    }
  }

  for (int i = 0; i < (int) array.size(); i += 16) {   // Display the result
    printf("%3i: %i\n", i, array[i]);
  }

  return 0;
}
