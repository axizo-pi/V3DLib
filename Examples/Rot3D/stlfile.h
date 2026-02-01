#ifndef V3DLIB_ROT3D_STLFILE_H
#define V3DLIB_ROT3D_STLFILE_H
#include "scalar.h"
#include <string>

void load_stl(std::string const &filename);

int stl_num_coords();
void stl_load_data(ScalarData &data);
void stl_save_data(ScalarData &data, bool save_file = false);

#endif // V3DLIB_ROT3D_STLFILE_H
