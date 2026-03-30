#include "model.h"
#include "scalar.h"
#include "kernel.h"
#include "Support/Timer.h"
#include "Support/Helpers.h"  // to_file()
#include "vc4/RegisterMap.h"
#include "LibSettings.h"
#include "settings.h"

using namespace Log;


void init_platform() {
  // Disable the cache
  if (Platform::compiling_for_vc4()) {
    // Disable L2 cache: this ensure that DMA and TMU can work together
    RegisterMap::L2Cache_enable(false);
    warn << "L2CacheEnabled(): " << RegisterMap::L2CacheEnabled();
  }

  // Disable TMU load
  //LibSettings::tmu_load tmu(false);  // vc4 mbox error
}


///////////////////////////////////////////////////////////////
// Main 
///////////////////////////////////////////////////////////////

/**
 *
 * ==================================================
 *
 * Examinations
 * ------------
 *
 * Done on 20260330.
 * Clearly something is different now since last check (too long ago).
 * Perhaps memory access, which I tinkered on.
 *
 * `barrier()` works but it kills the performance, for both
 * `v3d` and `vc4`.
 *
 * ## Pi3, vc4
 *
 * | barrier | QPU's | TMU | L2 Cache | Ouput OK | time | mbox error | Comment |
 * | ------- | ----- | --- | -------- | -------- | ---- | ---------- | ------- |
 * | yes     |  1    | yes | enabled  | yes      | 56s  | no,  5     |         |
 * | "       | 12    | "   | "        | "        | 50s  | yes, 4     |         |
 * | "       |  1    | "   | disabled | "        | 36s  | no,  5     |         |
 * | "       | 12    | "   | disabled | "        | 35s  | no,  5     |         |
 * | "       |  1    | no  | disabled | NO       | 65s  | no,  1     |         |
 * | "       | 12    | no  | disabled | NO       | -    | yes, 1     | mbox error consistent at 1         |
 * | no      |  1    | "   | enabled  | yes      | 56s  | no,  5     |         |
 * | "       | 12    | "   | "        | "        | 34s  | no,  5     |         |
 * | "       |  1    | "   | disabled | "        | 36s  | no,  5     |         |
 * | "       | 12    | "   | disabled | "        | 19s  | no,  5     |         |
 * | "       |  1    | no  | enabled  | NO       | 83s  | no,  2     |         |
 * | "       | 12    | no  | enabled  | NO       | 62s  | no,  2     |         |
 * | "       |  1    | no  | disabled | NO       | 62s  | no,  2     |         |
 * | "       | 12    | no  | disabled | NO       | 65s  | no,  2     |         |
 *
 * ### Conclusions
 *
 * - DMA load does _not_ give good results
 * - Disabling L2 cache with TMU load actually _increases_ performance
 * - barrier has no effect; it can only decrease performance
 * 
 */
int main(int argc, const char *argv[]) {
  settings.init(argc, argv);
  init_platform();
  warn << "Num qpu's: " << settings.num_qpus;

  Image img(512, 512);
  img.set_conversion_factor(IMG_CONVERSION_FACTOR);

  if (settings.kernel == 1) {
    //
    // Run scalar version
    //
    warn << "Run scalar kernel";
    Timer timer("Scalar timer", true);

    init_orbital_entities();
    scalar_run(img);

    // The plot image is always filled in,
    // output is optional.
    if (settings.output_orbits) {
      img.save("gravity.bmp");
    }
  } else {
    //
    // Run kernel version
    //
    warn << "Run GPU kernel";
    init_orbital_entities();
    Model m;
    m.init();
    m.plot();

    auto k = compile(kernel_gravity, settings);
    to_file("barrier_kernel.txt", k.dump());
    k.setNumQPUs(settings.num_qpus);

    k.load(
      &m.x, &m.y, &m.z,
      &m.v_x, &m.v_y, &m.v_z,
      &m.acc_x, &m.acc_y, &m.acc_z,
      &m.mass,
      NUM
    );

    {
      Timer timer("QPU timer", true);

      double t = 0;
      while (t < t_end) {
        k.run();
        m.plot();
  
        t += BATCH_STEPS*dt;
      }

      //warn << m.dump_acc();
      //warn << m.dump_pos();

      // The plot image is always filled in, output is optional.
      if (settings.output_orbits) {
        m.save_img();
      }
    }
  }

  return 0;
}
