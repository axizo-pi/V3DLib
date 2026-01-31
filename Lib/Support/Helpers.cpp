#include "Helpers.h"
#include "Support/basics.h"
#include <chrono>
#include <thread>

/** \file 
 * Helper Functions
 * ================
 *
 * Collection of useful functions which are unrelated to the object of the library.
 */

using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono;      // nanoseconds, system_clock, seconds

namespace V3DLib {

/**
 * Return a random float value between -1 and 1
 */ 
float random_float() {
  return  (1.0f*(((float) (rand() % 200)) - 100.0f))/100.0f;
}


std::string indentBy(int indent) {
  std::string ret;
  for (int i = 0; i < indent; i++) ret += " ";
  return ret;
}


void to_file(std::string const &filename, std::string const &content) {
  assert(!filename.empty());
  assert(!content.empty());

  FILE *f = fopen(filename.c_str(), "w");
 	assert (f != nullptr);
	fprintf(f, content.c_str());
	fclose(f);
}


void sleep(int sec) {
  sleep_for(seconds(sec));
}

}  // namespace V3DLib
