#ifndef _INCLUDE_MANDELBROT_SETTINGS
#define _INCLUDE_MANDELBROT_SETTINGS
#include "Support/Settings.h"

struct MandRange {
	MandRange() = default;

	MandRange(float tl_re, float tl_im, float br_re, float br_im) :
  	topLeftReal(tl_re),
  	topLeftIm(tl_im),
  	bottomRightReal(br_re),
  	bottomRightIm(br_im)
	{}

  float offsetX() const;
  float offsetY() const;

	MandRange& operator=(const MandRange &rhs);

	std::string dump() const;

  int numStepsWidth;
  int numStepsHeight;

  float topLeftReal     = -2.5f;
  float topLeftIm       = 2.0f;
  float bottomRightReal = 1.5f;
  float bottomRightIm   = -2.0f;
};


struct MandSettings : public V3DLib::Settings, public MandRange {
  const int ALL = 3;

  int    kernel;
  bool   output_grey;
  bool   output_color;
  int    num_iterations;

  int count;

  int num_items() const;
	static std::vector<const char *> const kernels();

  MandSettings();

  bool init_params() override;
};

void settings_init(int argc, const char *argv[]);
MandSettings const &settings();

#endif //  _INCLUDE_MANDELBROT_SETTINGS
