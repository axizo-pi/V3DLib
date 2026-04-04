/**
 * Examine how the kernel parameters (uniforms) are handled
 * for compile- and runtime-handling.
 */
#include <V3DLib.h>
#include "Support/Settings.h"
#include "Support/Helpers.h"  // to_file()

using namespace V3DLib;
using namespace Log;


void kernel_params(Float::Ptr out, Int::Ptr in, Float mult) {
	//*out = toFloat(Int(*in)) * mult)) * 1.25f;
	*out = toFloat(Int(*in)) * mult * 1.25f;
}


void dump_result(Float::Array &out) {
	std::string tmp;
  for (int i = 0; i < (int) out.size(); i++) {
		tmp << out[i] << ", ";
  }

	warn << "Result: " << tmp;
}


int main(int argc, const char *argv[]) {
  float mult = 1.5;
  Int::Array   in(16);
  Float::Array out(16);

	//================================================
	warn << " ";
	warn << "Compiling kernel k\n";

  auto k = compile(kernel_params);
	to_file("kernel_params.txt", k.dump());

	//================================================
	warn << " ";
	warn << "Compiling kernel k2\n";

  BaseKernel k2 = compile(kernel_params);
	to_file("kernel_params_2.txt", k2.dump());

	//================================================
	warn << " ";
	warn << "Running kernel k\n";

  in.fill(2);
  out.fill(-2);
  k.load(&out, &in, mult).run();
	dump_result(out);

	//================================================
	warn << " ";
	warn << "Running kernel k2\n";

	mult = 6;
  in.fill(4);
  out.fill(-1);
  k2.load(&out, &in, mult).run();
	dump_result(out);

  return 0;
}
