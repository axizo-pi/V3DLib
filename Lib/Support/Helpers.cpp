#include "Helpers.h"
#include "Support/basics.h"

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

}  // namespace V3DLib
