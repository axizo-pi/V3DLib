#ifndef _INCLUDE_RNNSUPPORT_HELPERS_H
#define _INCLUDE_RNNSUPPORT_HELPERS_H
#include "Source/Float.h"
#include "Support/Settings.h"
#include <string>

float frand();
unsigned rrand();
float frrand();
unsigned frrand_count();

std::string vector_dump(
	V3DLib::Float::Array const &src,
	int  size,
	int  start_index = 0,
	bool output_int = false
);


V3DLib::Settings &settings();

#endif // _INCLUDE_RNNSUPPORT_HELPERS_H
