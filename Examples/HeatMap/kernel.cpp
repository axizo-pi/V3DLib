#include "kernel.h"
#include <V3DLib.h>
#include "Kernels/Cursor.h"
#include "support.h"
#include "Support/Timer.h"
#include "Support/bmp.h"

using namespace V3DLib;

namespace {

/**
 * Performs a single step for the heat transfer
 */
void heatmap_kernel(Float::Ptr map, Float::Ptr mapOut, Int height, Int width) {
  Cursor cursor(width);

  For (Int offset = cursor.offset()*me() + 1,
       offset < height - cursor.offset() - 1,
       offset += cursor.offset()*numQPUs())

    Float::Ptr src = map    + offset*width;
    Float::Ptr dst = mapOut + offset*width;

    cursor.init(src, dst);

    // Compute one output row
    For (Int x = 0, x < width, x = x + 16)
      cursor.step([&x, &width] (Cursor::Block const &b, Float &output) {
        Float sum = b.left(0) + b.current(0) + b.right(0) +
                    b.left(1) +                b.right(1) +
                    b.left(2) + b.current(2) + b.right(2);

        output = b.current(1) - K * (b.current(1) - sum * 0.125);

        // Ensure left and right borders are zero
        Int actual_x = x + index();
        Where (actual_x == 0)
          output = 0.0f;
        End
        Where (actual_x == width - 1)
          output = 0.0f;
        End
      });
    End

    cursor.finish();
  End
}

} // anon namespace


/**
 * The edges always have zero values.
 * i.e. there is constant cold at the edges.
 */
void run_kernel() {
  // Allocate and initialise input and output maps
  Float::Array mapA(settings.SIZE);
  Float::Array mapB(settings.SIZE);
  mapA.fill(0.0f);
  mapB.fill(0.0f);

  // Inject hot spots
  inject_hotspots(mapA);

  // Compile kernel
  auto k = compile(heatmap_kernel, settings);
  k.setNumQPUs(settings.num_qpus);

  Timer timer("QPU run time");

  for (int i = 0; i < settings.num_steps; i++) {
    if (i & 1) {
			// Load the uniforms and invoke the kernel
      k.load(&mapB, &mapA, settings.HEIGHT, settings.WIDTH).run();
    } else {
			// Load the uniforms and invoke the kernel
      k.load(&mapA, &mapB, settings.HEIGHT, settings.WIDTH).run();

			if (settings.animate) {
				std::string filename;
				filename << (i/2) << "_heatmap.bmp";
  			output_bmp(mapB, settings.WIDTH, settings.HEIGHT, 255, filename.c_str(), false);
			}
    }
  }

  timer.end(!settings.silent);

	if (!settings.animate) {
	  // Output results
  	output_bmp(mapB, settings.WIDTH, settings.HEIGHT, 255, "heatmap.bmp", false);
	}
}


