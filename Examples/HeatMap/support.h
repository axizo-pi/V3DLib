#ifndef V3DLIB_HEATMAP_SUPPORT_H
#define V3DLIB_HEATMAP_SUPPORT_H
#include "settings.h"
#include <stdlib.h>  // srand()


const float K = 0.25;   // Heat dissipation constant

// ============================================================================
// Helper function
// ============================================================================

template<typename Arr>
void inject_hotspots(Arr &arr) {
  srand(0);

  for (int i = 0; i < settings.num_points; i++) {
    int t = rand() % 256;
    int x = 1 + rand() % (settings.WIDTH  - 2);
    int y = 1 + rand() % (settings.HEIGHT - 2);
    arr[y*settings.WIDTH + x] = (float) (1000*t);
  }
}

#endif // V3DLIB_HEATMAP_SUPPORT_H
