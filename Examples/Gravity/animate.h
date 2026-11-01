#ifndef _V3DLIB_GRAVITY_ANIMATE_H
#define _V3DLIB_GRAVITY_ANIMATE_H
#include "Support/bmp.h"
#include <functional>

void save_animate(std::function<void(Image &img)> plot_image, bool force = false);

#endif  // _V3DLIB_GRAVITY_ANIMATE_H
