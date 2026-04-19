#include "doctest.h"
#include "Support/Helpers.h"
#include "support/support.h"
#include <iostream>

using namespace V3DLib;

namespace {

/**
 * @brief Retrieve last num lines of a logfile
 *
 * The timestamp and level prefix is removed from all lines
 */
std::vector<std::string> last_log_lines(std::string const &path, int num_lines) {
	std::vector<std::string> ret;

	auto vec = load_file_vec(path);
	//std::cout << "logfile:\n";
	for (int i = (int) vec.size() - num_lines; i < (int) vec.size(); ++i) {
		auto str = vec[i];
		str = split(str, "DEBUG: ")[1];
		ret << str;

		//std::cout << "  " << str << "\n";
	}

	return ret;
}

}  // anon namespace


TEST_CASE("Test Logging [log]") {
	std::cout << "Hello testing logging.\n";

	std::string prev_logdir  = Log::log_dir();
	std::string prev_logfile = Log::log_file();
	Log::set_log_dir(test_path());

	std::string logfile = "log_test.log";
 	Log::set_log_file(logfile);

  SUBCASE("Test int modifiers") {
		const int NUM_LINES = 2;

		std::vector<std::string> expected = {
		 	"2 0x2 2",
		 	"3 0x3 0x3 3 0x3 3 3",
		};
		REQUIRE((int) expected.size() == NUM_LINES);

		cdebug << 2 << " " << hex << 2 << " " << 2;
		cdebug << 3 << " " 
			     << global << hex << 3 << " " << 3 << " " 
			     << dec << 3 << " " << 3 << " " 
					 << global << dec << 3 << " " << 3;


		auto vec = last_log_lines(test_path() + "/" + logfile, NUM_LINES);
		REQUIRE((int) vec.size() == NUM_LINES);

		// Better to check each line separately, for better specification error
		for (int i = 0; i < (int) vec.size(); ++i) {
			REQUIRE(vec[i] == expected[i]);
		}
	}

  SUBCASE("Test float modifiers") {
		const int NUM_LINES = 3;

		std::vector<std::string> expected = {
			"2.000000e+01 20.000000 2.000000e+01",
			"0.998000 0.998000 9.980000e-01 0.998000",
			"0.010000 1.000000e-02 0.010000 1.000000e-02"
		};
		REQUIRE((int) expected.size() == NUM_LINES);

		cdebug << 2.0e1f << " " << fixed << 2.0e1f << " " << 2.0e1f;
		cdebug << global << fixed 
					 << 0.998f << " " 
					 << 0.998f << " " 
			     << scientific << 0.998f << " "
					 << 0.998f;

		// Global modifier should be retained over logging calls
		cdebug << 0.01f << " " 
					 << global << scientific 
					 << 0.01f << " " 
					 << fixed
					 << 0.01f << " " 
					 << 0.01f;

		auto vec = last_log_lines(test_path() + "/" + logfile, NUM_LINES);
		REQUIRE((int) vec.size() == NUM_LINES);

		// Better to check each line separately, for better specification error
		for (int i = 0; i < (int) vec.size(); ++i) {
			INFO("vec index: " << i);
			REQUIRE(vec[i] == expected[i]);
		}
	}

	Log::set_log_file(prev_logfile);
	Log::set_log_dir(prev_logdir);
}
