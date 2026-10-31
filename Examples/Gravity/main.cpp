/**
 * /file
 *
 * ## Timing `vc7`
 *
 * - Default 250 years
 * - Ordered model with equidistant asteroids in the orbital plane
 *
 * |--------|--------|-----------|----------|-----------------------------------------|
 * | Kernel | #QPU's | #Entities | Time (s) | Comment
 * |--------|--------|-----------|----------|-----------------------------------------|
 * | cpu    |      - |       128 |  110.814 |
 * | qpu    |      1 |         " |   53.165 |
 * |        |     16 |         " |    8.731 |
 * | cpu    |      - |       288 |  561.344 |
 * | qpu    |      1 |         " |  228.202 |
 * |        |     16 |         " |   23.355 |
 * | cpu    |      - |      1024 |        - | Previous test 20260127 was over 2 hours |
 * | qpu    |      1 |         " |  |
 * |        |     16 |         " |  205.503 |
 * |--------|--------|-----------|----------|-----------------------------------------|
 */
#include "model.h"
#include "scalar.h"
#include "kernel.h"
#include "Support/Helpers.h"  // to_file()
#include "vc4/RegisterMap.h"
#include "LibSettings.h"
#include "settings.h"
#include "global/Timer.h"

using namespace Log;

/**
 * @brief Platform settings for code generation
 *
 * These settings are `vc4` only.
 *
 * Empirically, following settings are best:
 * - Disable L2 cache (performance)
 * - Use TMU load (DMA load gives errors)
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
  //
  // Disable the L2 cache - vc4 only
  // This ensures that DMA and TMU can work together
  //
  if (Platform::compile::for_vc4() && settings.run_type == QPU) {
    LibSettings::L2Cache_enable(false);
  }

  // Set TMU/DMA load - does nothing for vc7
  LibSettings::use_tmu_for_load(true);
}


///////////////////////////////////////////////////////////////
// Main 
///////////////////////////////////////////////////////////////

/**
 * @brief Entry point for program.
 *
 * ==================================================
 *
 * - cpu: works fine on all platforms
 * - qpu: Only works properly for vc7
 * - interpreter: Works but does less iterations than cpu/qpu for default 250 years.
 *                Completes for 32 years.
 *
 *   A multitude of the following logs:
 *
 *       load_from_heap(): index does not have all same values
 *
 *  This appears to be benign.
 *
 * - emulator: not tested
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
    if (settings.do_animate) {
      warn << "Animate not enabled for cpu";
    }

    Timer timer("Scalar timer", true);

    init_orbital_entities();
    scalar_run(img);

    // The plot image is always filled in, output is optional.
    if (settings.output_orbits) {
      img.save("gravity.bmp");
    }
  } else {
    //
    // Run kernel version
    //
    warn << "Run GPU kernel";

    init_orbital_entities();
    Model m;  // Orbit image
    m.init();
    m.plot();

    auto k = compile(gravity_kernel, settings);
    //to_file("Gravity_kernel.txt", k.dump());
    //to_file("Gravity_kernel.cpp", k.compile().dump_kernel_hex());
    k.setNumQPUs(settings.num_qpus);

    k.load(
      &m.x, &m.y, &m.z,
      &m.v_x, &m.v_y, &m.v_z,
      &m.acc_x, &m.acc_y, &m.acc_z,
      &m.mass,
      settings.num_entities
    );

    {
      timers.start("QPU gravity");

      double t = 0;
      while (t < t_end) {
        k.run();

        m.plot();                     // The orbit image is always updated 
        m.save_animate();
        t += batch_steps()*dt;
      }

      m.save_orbits();
      timers.stop("QPU gravity");
    }

    m.save_animate(true);
  }

  timers.end(true);
  return 0;
}
