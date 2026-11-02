#ifndef V3DLIB_GRAVITY_SETTINGS_H
#define V3DLIB_GRAVITY_SETTINGS_H
#include "Support/Settings.h"

struct GravitySettings : public V3DLib::Settings {

  bool do_animate;
  bool output_orbits;
  int  kernel;
  int  num_entities;
  int  years;
  std::string model;

  GravitySettings();

  bool init_params() override;
};

extern struct GravitySettings settings;

#endif // V3DLIB_GRAVITY_SETTINGS_H
