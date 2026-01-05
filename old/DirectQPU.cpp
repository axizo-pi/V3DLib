/**
 * Test if QPU and CPU can communicate via main memory.
 *
 * Conclusion: doesn't work. Most likely because QPU uses cache
 */
#include "V3DLib.h"
#include "Support/Settings.h"
#include "Support/basics.h"
#include "Support/Helpers.h"
#include "global/log.h"

using namespace V3DLib;
using namespace Log;

Settings settings;

/**
 * DOESN'T WORK: Looks like it doesn't even start
 */
void kernel(Int::Ptr p) {
	p -= index();
	p += me();

	Int::Ptr run_state = p;  p.inc();
	Int::Ptr exec      = p;  p.inc();
	Int::Ptr result    = p;

	*run_state = 1;

	Int count = 1;
//	Int signal = *exec;
	Int signal = 1;

	//While (all(*exec == 0));   // Doesn't compile
	While (signal != 2); // && count < 100);  // Loop entered when adding second condition

		//If (all(signal == 1));
		If (signal == 1);
			*result = 2*count*(me() +1);
			count++;

			*exec = 0;
		End;

		//signal = *exec;
		signal = 2;
	End;

	*exec = 0;
	*run_state = 2;
}


void dump_state(Int::Array &array) {
	std::string buf;
  for (int i = 0; i < (int) array.size(); i++) {
		if (i % 16 == 0) {
			buf << "\n" << i << ": ";
		}

		buf << array[i] << ", ";
  }

	warn << buf;
}


/**
 * NOT WORKING. At best, it is fickle.
 *
 * Hypotheses:
 * - Memory access issue, error when both QPU and CPU simultaneously access same memory
 * - In-built guard in QPU's for runaway loops
 */
int main(int argc, const char *argv[]) {
  settings.init(argc, argv);
  auto k = compile(kernel, settings); 
  k.setNumQPUs(3);

	// index  0: run state; 0: not started, 1: running, 2: done
	// index 16: set to 1 to execute
	// index 32: result
  Int::Array array(3*16);
  array.fill(0);

  k.load(&array);
	k.run(false);

	//sleep(1);
	//dump_state(array);

	// Looks like you have to fill in for ALL assigned QPUS
	// If the values are set together, it works
/*	
	array[16] = 1;
	array[16 + 1] = 1;
	array[16 + 2] = 1;
	array[16 + 3] = 1;
	array[16 + 4] = 1;
	array[16 + 5] = 1;
*/
	sleep(1);
	dump_state(array);

	//sleep(1);
	//dump_state(array);
	
	k.wait_complete();
	dump_state(array);

  return 0;
}
