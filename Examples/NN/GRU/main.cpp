#include "Support/Timer.h"
#include "BaseKernel.h"
#include <iostream>

using namespace V3DLib;

extern void train_main();
extern void test_main(std::string const &epoch, std::string const &loss);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Require an action:\n"
              << "  ./GRU train\n"
              << "  ./GRU test <epoch> <loss>\n"
              << std::endl;
    return 0;
  }

  std::string action = argv[1];

  if (action == "train") {
    std::cout << "Doing train" << std::endl;
    train_main();
  } else if (action == "test") {
    if (argc < 4) {
      std::cout << "test expects two parameters <epoch> and <loss>" << std::endl;
      return 0;
    }

    std::cout << "Doing test" << std::endl;

    std::string epoch = argv[2];
    std::string loss  = argv[3];

    test_main(epoch, loss);
  } else {
    std::cout << "Can't perform action '" << action << "'" << std::endl;
  }

  timers.end(); //true); // Param to enable display min/max
	std::cout << "QPU Call Count: " << V3DLib::BaseKernel::qpu_call_count() << std::endl;
  return 0;
}
