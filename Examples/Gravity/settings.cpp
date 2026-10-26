#include "settings.h"
#include "defaults.h"
#include "global/Log.h"

using namespace Log;

namespace {

CmdParameters params = {
  "Gravity Simulator\n"
  "\n"
  "Gravity is calculated for every day in the simulation.\n",
  {{
		"Output orbit image",
		{ "-orbits", "-o" },
    ParamType::NONE,
    "Output an image of the combined orbits of all entities."
	 }, {
    "Kernel",
    "-k=",
		{ "gpu", "cpu" },
    "Select the kernel to use"
	 }, {
		"Number of years",
		{ "-years=", "-y=" },
    ParamType::POSITIVE_INTEGER,
    "Set the number of years to run the simulation",
		250
  }, {
    "Animate the results",
    { "-a", "-animate"},
		ParamType::NONE,
    "Create an indexed sequence of bitmaps for the simulation\n"
		"<TODO>"
	}}
};

} // anon namespace

GravitySettings::GravitySettings() : Settings(&params, true) {}

bool GravitySettings::init_params() {
  auto const &p = parameters();

  do_animate    = p["Animate the results" ]->get_bool_value();
  output_orbits = p["Output orbit image" ]->get_bool_value();
  kernel        = p["Kernel"]->get_int_value();
	set_num_years(p["Number of years"]->get_int_value());

  return true;
}


struct GravitySettings settings;
