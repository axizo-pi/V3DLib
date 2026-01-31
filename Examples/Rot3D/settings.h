#ifndef V3DLIB_ROT3D_SETTINGS_H
#define V3DLIB_ROT3D_SETTINGS_H
#include "Support/Settings.h"

struct Rot3DSettings : public V3DLib::Settings {
  const float THETA = (float) 3.14159;  // Rotation angle in radians

  int   kernel;
  bool  show_results;
  int   num_vertices;

	bool  show_info;
	float rot_x;
	float rot_y;
	float rot_z;

  Rot3DSettings();

  bool init_params() override;
};

extern std::vector<const char *> const kernel_id;
extern struct Rot3DSettings settings;

#endif //  V3DLIB_ROT3D_SETTINGS_H
