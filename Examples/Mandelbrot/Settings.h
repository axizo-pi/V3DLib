#ifndef _INCLUDE_MANDELBROT_SETTINGS
#define _INCLUDE_MANDELBROT_SETTINGS
#include "Support/Settings.h"

struct MandSettings : public V3DLib::Settings {
  const int ALL = 3;

  int    kernel;
  bool   output_grey;
  bool   output_color;
  int    num_iterations;

  int numStepsWidth;
  int numStepsHeight;

  int count;

  // Initialize constants for the kernels
  const float topLeftReal     = -2.5f;
  const float topLeftIm       = 2.0f;
  const float bottomRightReal = 1.5f;
  const float bottomRightIm   = -2.0f;

/*
  // Nice zoom-in range, used for testing
  const float topLeftReal     = -2.0f;
  const float topLeftIm       = 0.75f;
  const float bottomRightReal = -1.0f;
  const float bottomRightIm   = -0.25f;
*/

  int num_items() const;
  float offsetX() const;
  float offsetY() const;
	static std::vector<const char *> const kernels();

  MandSettings();

  bool init_params() override;

};

void settings_init(int argc, const char *argv[]);
MandSettings const &settings();

#endif //  _INCLUDE_MANDELBROT_SETTINGS
