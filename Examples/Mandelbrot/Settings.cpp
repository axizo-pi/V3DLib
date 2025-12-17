#include "Settings.h"

namespace {

std::vector<const char *> const _kernels = { "multi", "cpu" };  // Order important! First is default, 'all' must be last



CmdParameters params = {
  "Mandelbrot Generator\n"
  "\n"
  "Calculates Mandelbrot for a given region and outputs the result as a PGM bitmap file.\n"
  "The kernel is compute-bound, the calculation time dominates over the data transfer during execution.\n"
  "It is therefore an indicator of compute speed, "
  "and is used for performance comparisons of platforms and configurations.\n"
	"\n"
	"NOTE: due to technical limitations, the minimal number of selected QPU's for vc4 is 4.\n",

  {{
    "Kernel",
    "-k=",
    _kernels,
    "Select the kernel to use"
  }, {
    "Output PGM file",
    "-pgm",
    ParamType::NONE,   // Prefix needed to disambiguate
    "Output a (grayscale) PGM bitmap of the calculation results.\n"
    "A PGM bitmap named 'mandelbrot.pgm' will be created in the current working directory.\n"
    "Creating a bitmap takes significant time, and will skew the performance results\n",
  }, {
    "Output PPM file",
    "-ppm",
    ParamType::NONE,
    "Output a (color) PPM bitmap of the calculation results.\n"
    "A PPM bitmap named 'mandelbrot.ppm' will be created in the current working directory.\n"
    "Creating a bitmap takes significant time, and will skew the performance results\n",
  }, {
    "Number of steps",
    "-steps=",
    ParamType::POSITIVE_INTEGER,
    "Maximum number of iterations to perform per point",
    1024
  }, {
    "Dimension",
    "-dim=",
    ParamType::POSITIVE_INTEGER,
    "Number of steps (or 'pixels') in horizontal and vertical direction",
    1024
  }, {
    "Count",
    { "-c=", "-count=" },
    ParamType::POSITIVE_INTEGER,
    "Perform the operation this number of times",
    1
  }}
};

MandSettings _settings;

}  // anon namespace

MandSettings::MandSettings() : Settings(&params, true) {}


int MandSettings::num_items() const { return numStepsWidth*numStepsHeight; }
float MandSettings::offsetX() const { return (bottomRightReal - topLeftReal  )/((float) numStepsWidth  - 1); }
float MandSettings::offsetY() const { return (topLeftIm       - bottomRightIm)/((float) numStepsHeight - 1); }


bool MandSettings::init_params() {
  auto const &p = parameters();

  kernel         = p["Kernel"]->get_int_value();
  output_pgm     = p["Output PGM file"]->get_bool_value();
  output_ppm     = p["Output PPM file"]->get_bool_value();
  num_iterations = p["Number of steps"]->get_int_value();
  numStepsWidth  = p["Dimension"]->get_int_value();
  numStepsHeight = p["Dimension"]->get_int_value();
  count          = p["Count"]->get_int_value();

  return true;
}

std::vector<const char *> const MandSettings::kernels() { return _kernels; }


void settings_init(int argc, const char *argv[]) {
  _settings.init(argc, argv);
}

MandSettings const &settings() { return _settings; }

