#include "settings.h"
#include "global/log.h"

using namespace Log;

namespace {

CmdParameters params = {
  "Gravity Simulator",

  {{
		"Output orbit image",
    "-orbits",
    ParamType::NONE,
    "Output an image of the combined orbits of all entities"
	}}
};

} // anon namespace

GravitySettings::GravitySettings() : Settings(&params, true) {}

bool GravitySettings::init_params() {
  auto const &p = parameters();

  output_orbits = p["Output orbit image" ]->get_bool_value();

  return true;
}


struct GravitySettings settings;
