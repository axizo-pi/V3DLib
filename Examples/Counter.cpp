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


void kernel(Int x_count, Int y_count, Int::Ptr result) {
	Int count = 0;

  For (Int x = 0, x < x_count, x++)
  	For (Int y = 0, y < y_count, y++)
		End

	//	Where(count >= 0)
			count += 1;
	//	End
		*result = count;
  End

}


int main(int argc, const char *argv[]) {
	Settings settings;
  settings.init(argc, argv);

  auto k = compile(kernel, settings);
  k.setNumQPUs(settings.num_qpus);

	int x_count = 3000000;
	Int::Array result(16);

	k.load(x_count, 1 /*2280*/, &result);

  Timer timer;
	k.run();
  timer.end(!settings.silent);

  for (int i = 0; i < (int) result.size(); i++)
    printf("%i: %d\n", i, result[i]);

  return 0;
}
