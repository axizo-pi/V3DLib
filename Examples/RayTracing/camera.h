#ifndef CAMERA_H
#define CAMERA_H
//==============================================================================================
// Originally written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication
// along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==============================================================================================
#include "ray.h"
#include "color.h"
#include "global.h"

class hittable;

class camera: public ViewPort {
  public:
    int    max_depth         = 10;   // Maximum number of ray bounces into scene

    void initialize();
    int  init_rays();
    void render(const hittable& world, PPM &ret); 

  private:
    double pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    point3 center;               // Camera center

    color ray_color(const ray& r, int depth, const hittable& world, int ray_index = -1, bool do_qpu = false) const;
};

#endif
