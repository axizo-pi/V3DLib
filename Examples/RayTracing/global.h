#ifndef _RAYTRACING_GLOBAL_H
#define _RAYTRACING_GLOBAL_H
#include "ray.h"
#include <functional>


/**
 * @brief Indicator for run mode.
 *
 * There are three options for running:
 *
 * - RunQPU    - Run QPU calculation only
 * - RunScalar - Run CPU calculation only
 * - RunCheck  - Run both QPU and CPU and compare output where checks are enabled
 */
enum RunMode {
  RunQPU,
  RunScalar,
  RunCheck
};


class ViewPort {
public:
  double vfov     = 90;              // Vertical view angle (field of view)
  point3 lookfrom = point3(0,0,0);   // Point camera is looking from
  point3 lookat   = point3(0,0,-1);  // Point camera is looking at
  vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction

  vec3   pixel_delta_u;        // Offset to pixel to the right
  vec3   pixel_delta_v;        // Offset to pixel below
  vec3   u, v, w;              // Camera frame basis vectors

  double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus

  // Derived values, originally local to init-method
  double viewport_height;
  double viewport_width;
  vec3 viewport_u;    // Vector across viewport horizontal edge
  vec3 viewport_v;  // Vector down viewport vertical edge

  void init();
};


namespace global {

RunMode run_mode();  

void aspect_ratio(double val);
int  image_width();
void image_width(int val);
int  image_height();
void samples_per_pixel(int val);
int  samples_per_pixel();
int num_rays();

void defocus_init(ViewPort const &vp, point3 const &in_center);
ray get_ray(int i, int j);

} // namespace global


class RayIterator {
public: 	
  bool done() const;
  bool next(std::function<bool(int index, ray r)> f);

private:
  int i = -1;      // Index over width, -1 indicates not run yet
  int j = 0;       // Index over height
  int sample = 0;

  bool inc();
};

extern RayIterator ray_iterator;


class PPM {
using color = vec3;

public:
	PPM();
	void write_color(const color& pixel_color);
	void write() const;

private:
	std::string ret;
};	

#endif // _RAYTRACING_GLOBAL_H
