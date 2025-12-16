#include "V3DLib.h"
#include "Support/Settings.h"
#include "Support/basics.h"
#include "Support/Helpers.h"
#include "global/log.h"

using namespace V3DLib;
using namespace Log;

Settings settings;

void kernel(Int::Ptr p) {
	p -= index();
	p += me();

	Int::Ptr run_state = p;  p.inc();
	Int::Ptr exec      = p;  p.inc();
	Int::Ptr result    = p;

	*run_state = 2;

	*exec = 4;

	Int count = 1;
	*result = count*(me() +1);
	count++;

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


int main(int argc, const char *argv[]) {
  settings.init(argc, argv);
  auto k = compile(kernel, settings); 
  k.setNumQPUs(2);

	// index  0: run state; 0: not started, 1: running, 2: done
	// index 16: set to 1 to execute
	// index 32: result
  Int::Array array(3*16);
  array.fill(0);

  k.load(&array).run();

	dump_state(array);
	sleep(1);
	dump_state(array);


  return 0;
}
