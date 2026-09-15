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
#include "rtweekend.h"
#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "qpu.h"
#include "./global.h"
#include "global/log.h"
#include "Support/Timer.h"
#include "Support/dump.h"   // bitdiff_stats::dump()

using namespace V3DLib;
using namespace Log;


int main() {
  Log::enable_log_file();
  Log::info << "\n"
            << "===================\n"
            << " Running RayTrace\n"
            << "===================";

  timers.start("Init");
    qpu::kernels_init();

    hittable_list world;

    //
    // Initialize the spheres
    //
    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    camera cam;

    global::aspect_ratio(16.0 / 9.0);
    global::image_width(64); //1200;
    global::samples_per_pixel(10);

    cam.max_depth  = 20;
    cam.vfov       = 20;
    cam.lookfrom   = point3(13,2,3);
    cam.lookat     = point3(0,0,0);
    cam.vup        = vec3(0,1,0);
    cam.focus_dist = 10.0;

    cam.initialize();

  int num_spheres = spheres::size();
  qpu::init_arrays(num_spheres);
  spheres::init();

  warn << "num rays: " << global::num_rays();

  timers.stop("Init");

  {
    timers.start("Run");

    PPM ppm;

    while (!ray_iterator.done()) {
      if (cam.init_rays() > 0) {
        if (global::run_mode() != RunScalar) {
          qpu::run_kernel();
        }
        cam.render(world, ppm);
      }
    }

    ppm.write();

    timers.stop("Run");
  }

  // Finalize output
  std::clog << "\rDone.                 \n";
  timers.end();
  qpu::end();
  bitdiff_stats::dump();
}
