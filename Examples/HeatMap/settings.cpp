#include "settings.h"
#include <CmdParameters.h>

namespace {

CmdParameters params = {
  "HeatMap Example\n"
  "\n"
  "This example models the heat flow across a 2D surface.\n"
  "The output is a bmp-bitmap with the final state of the surface.\n"
  "\n"
  "The edges are set at zero temperature, and a number of hot points are placed randomly over the surface.\n"
  "The lower border can be broader than 1 pixel, depending on the number of QPU's running. This is due to\n"
  "preventing image overrun."
  "\n",
  {{
    "Kernel",
    "-k=",
    {"vector", "scalar"},  // First is default
    "Select the kernel to use"
  }, {
    "Number of steps",
    "-steps=",
    POSITIVE_INTEGER,
    "Set the number of steps to execute in the calculation",
    1500
  }, {
    "Number of points",
    "-points=",
    POSITIVE_INTEGER,
    "Set the number of randomly distributed hot points to start with",
    10
  }, {
    "Animate the results",
    { "-a", "-animate"},
		ParamType::NONE,
    "Create an indexed sequence of bitmaps for the simulation\n"
		"A bitmap is outputted for every second iteration, eg. 750 images for the default number of steps"
  }}
};

} // anon namespace


HeatMapSettings::HeatMapSettings() : Settings(&params, true) {}

struct HeatMapSettings settings;
