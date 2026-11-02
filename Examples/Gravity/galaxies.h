#ifndef _V3DLIB_GRAVITY_SETTINGS_H
#define _V3DLIB_GRAVITY_SETTINGS_H
#include "Support/bmp.h"

namespace galaxies {

void determine_factor(Image const &img);
void step();
void draw(Image &img);

}  // namespace galaxies

#endif // _V3DLIB_GRAVITY_SETTINGS_H
