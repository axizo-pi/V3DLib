#ifndef V3DLIB_GRAVITY_SETTINGS_H
#define V3DLIB_GRAVITY_SETTINGS_H
#include "Support/Settings.h"

struct GravitySettings : public V3DLib::Settings {

	bool output_orbits;

	GravitySettings();

  bool init_params() override;
};

extern struct GravitySettings settings;

#endif // V3DLIB_GRAVITY_SETTINGS_H
