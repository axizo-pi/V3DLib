#include "settings.h"
#include "defaults.h"
#include "global/log.h"

using namespace Log;

namespace {

CmdParameters params = {
  "Gravity Simulator\n"
  "\n"
  "Gravity is calculated for every day in the simulation\n",
  {{
		"Output orbit image",
    "-orbits",
    ParamType::NONE,
    "Output an image of the combined orbits of all entities"
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
	}}
};

} // anon namespace

GravitySettings::GravitySettings() : Settings(&params, true) {}

bool GravitySettings::init_params() {
  auto const &p = parameters();

  output_orbits = p["Output orbit image" ]->get_bool_value();
  kernel        = p["Kernel"]->get_int_value();
	set_num_years(p["Number of years"]->get_int_value());

  return true;
}


struct GravitySettings settings;
