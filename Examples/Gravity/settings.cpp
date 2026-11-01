#include "settings.h"
#include "defaults.h"
#include "global/global.h"

using namespace V3DLib;
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
  }, {
    "Number of entities",
    { "-e=", "-num_entities="},
    ParamType::POSITIVE_INTEGER,
    "Set the number of entities in the simulation\n"
    "The default value is the minimum value. The value must be a multiple of 32",
    32
  }, {
    "Entity model",
    { "-model="},
    { "ordered", "chaos", "galaxy"},
    "Select how the entities are initialized. Options\n"
    "  - ordered: Solar system with equidistant asteroids in the orbital plane\n"
    "  - chaos  : Solar system with random asteroid placement with random speeds\n"
    "  - galaxy : Stars organized galaxies, like the Galaxy screensaver"
  }}
};

} // anon namespace

GravitySettings::GravitySettings() : Settings(&params, true) {}

bool GravitySettings::init_params() {
  auto const &p = parameters();

  do_animate    = p["Animate the results"]->get_bool_value();
  output_orbits = p["Output orbit image" ]->get_bool_value();
  kernel        = p["Kernel"             ]->get_int_value();
  num_entities  = p["Number of entities" ]->get_int_value();
  model         = p["Entity model"       ]->get_string_value();

  set_num_years(p["Number of years"]->get_int_value());

  if (num_entities < 32) {
    throw Exception("Number of entities must be at least 32");
  }

  if (num_entities % 32 != 0) {
    throw Exception("Number of entities must be a multiple of 32");
  }

  if (model == "galaxy") {
    if (kernel == 0) {
      throw Exception("Gravity model not yet implemented for qpu");
    }

    throw Exception("Gravity model cpu TODO");
  }

  return true;
}


struct GravitySettings settings;
