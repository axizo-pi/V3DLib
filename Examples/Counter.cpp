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

		//*result = count;
  End

	*result = count;   // This never returns a value???
}


int main(int argc, const char *argv[]) {
	Settings settings;
  settings.init(argc, argv);

  auto k = compile(kernel, settings);
  k.setNumQPUs(settings.num_qpus);

	// vc7 testing: 
	//int x_count = 3529475;  // Ultimate limit, works once then fails
	//int x_count = 3529400;  // Fails intermittently
	//int x_count = 3529000;  // idem
	//int x_count = 3520000;  // idem
	//int x_count = 3500000;  // idem
	int x_count = 3400000;    // Worked 10x in a row; between values not tested
	Int::Array result(16);

	k.load(x_count, 1, &result); // vc7  with previous comments

  // vc6 testing:
  // y=100 times out
  // y=50 ends without a returned reason but timer > 10
  // y=30 hangs but works after reset
	//k.load(x_count, 20, &result);

  Timer timer;
	k.run();
  timer.end(!settings.silent);

  for (int i = 0; i < (int) result.size(); i++)
    printf("%i: %d\n", i, result[i]);

  return 0;
}
