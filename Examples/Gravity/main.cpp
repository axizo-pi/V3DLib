#include "model.h"
#include "scalar.h"
#include "kernel.h"
#include "Support/Timer.h"
#include "Support/Helpers.h"  // to_file()
#include "vc4/RegisterMap.h"
#include "LibSettings.h"
#include "settings.h"

using namespace Log;

/**
 * @brief Platform settings for code generation
 *
 * These settings are `vc4` only.
 *
 * Empirically, following settings are best:
 * - Disable L2 cache (performance)
 * - Use TMu load (DMA load gives errors
 *
 * Further for `vc4`:
 * - Use BATCH_STEPS == 1 only
 * - no barrier for `vc4`
 *
 * With these settings, `vc4` barrier works properly.
 * The performance, however, is dismal compared to `v3d`.
 *
 */
void init_platform() {
#ifdef QPU_MODE
  // Disable the cache - vc4 only
  if (Platform::compiling_for_vc4() && settings.run_type == QPU) {
    // Disable L2 cache: this ensure that DMA and TMU can work together
    LibSettings::L2Cache_enable(false);
  }
#endif // QPU_MODE  

  // Set TMU/DMA load - does nothing for vc7
  // WRONG: LibSettings::tmu_load tmu(false);
  LibSettings::use_tmu_for_load(true);  // true: use TMU load
}


///////////////////////////////////////////////////////////////
// Main 
///////////////////////////////////////////////////////////////

/**
 *
 * ==================================================
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
