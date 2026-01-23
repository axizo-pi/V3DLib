#include "scalar.h"
#include "support.h"
#include <string>
#include "Support/basics.h"
#include "Support/bmp.h"

namespace {

/**
 * One time step for the scalar kernel
 */
void scalar_step(float** map, float** mapOut, int width, int height) {
  for (int y = 1; y < height-1; y++) {
    for (int x = 1; x < width-1; x++) {
      float surroundings =
        map[y-1][x-1] + map[y-1][x]   + map[y-1][x+1] +
        map[y][x-1]   +                 map[y][x+1]   +
        map[y+1][x-1] + map[y+1][x]   + map[y+1][x+1];
      surroundings *= 0.125f;
      mapOut[y][x] = (float) (map[y][x] - (K * (map[y][x] - surroundings)));
    }
  }
}

} // anon namespace


void run_scalar() {
  float*  map      = new float [settings.SIZE];
  float*  mapOut   = new float [settings.SIZE];
  float** map2D    = new float* [settings.HEIGHT];
  float** mapOut2D = new float* [settings.HEIGHT];

  // Initialise
  for (int i = 0; i < settings.SIZE; i++) {
    map[i] = mapOut[i] = 0.0;
  }

  for (int i = 0; i < settings.HEIGHT; i++) {
    map2D[i]    = &map[i*settings.WIDTH];
    mapOut2D[i] = &mapOut[i*settings.WIDTH];
  }

  inject_hotspots(map);

  // Simulate
  for (int i = 0; i < settings.num_steps; i++) {
    scalar_step(map2D, mapOut2D, settings.WIDTH, settings.HEIGHT);
    float** tmp = map2D;
		map2D = mapOut2D;
		mapOut2D = tmp;

		if ((i % 2) == 0 && settings.animate) {
			std::string filename;
			filename << (i/2) << "_heatmap.bmp";
  		output_bmp(mapOut, settings.WIDTH, settings.HEIGHT, 255, filename.c_str(), false);
		}
  }

	if (!settings.animate) {
	  // Output results
  	output_bmp(mapOut, settings.WIDTH, settings.HEIGHT, 255, "heatmap.bmp", false);
	}
}
