#ifndef V3DLIB_HEATMAP_SETTINGS_H
#define V3DLIB_HEATMAP_SETTINGS_H
#include "Support/Settings.h"
#include <string>

struct HeatMapSettings : public V3DLib::Settings {
  // Parameters
  const int WIDTH  = 512;           // Should be a multiple of 16 for QPU
  const int HEIGHT = 506;           // Should be a multiple of num_qpus for QPU
  const int SIZE   = WIDTH*HEIGHT;  // Size of 2D heat map

  int    kernel;
	std::string kernel_name;
  int    num_steps;
  int    num_points;
  bool   animate;

  HeatMapSettings();

  bool init_params() override {
    auto const &p = parameters();

    kernel      = p["Kernel"             ]->get_int_value();
    kernel_name = p["Kernel"             ]->get_string_value();
    num_steps   = p["Number of steps"    ]->get_int_value();
    num_points  = p["Number of points"   ]->get_int_value();
    animate     = p["Animate the results"]->get_bool_value();

    return true;
  }
};

extern struct HeatMapSettings settings;


#endif // V3DLIB_HEATMAP_SETTINGS_H
