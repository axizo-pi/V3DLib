/**
 * /file
 *
 * ## Timing `vc7`
 *
 * - 20261031
 * - Default 250 years
 * - Ordered model with equidistant asteroids in the orbital plane
 *
 * |--------|--------|-----------|----------|-----------------------------------------|
 * | Kernel | #QPU's | #Entities | Time (s) | Comment
 * |--------|--------|-----------|----------|-----------------------------------------|
 * | cpu    |      - |       128 |  110.814 |                                         |
 * | qpu    |      1 |         " |   53.165 |                                         |
 * |        |     16 |         " |    8.731 |                                         |
 * | cpu    |      - |       288 |  561.344 |                                         |
 * | qpu    |      1 |         " |  228.202 |                                         |
 * |        |     16 |         " |   23.355 |                                         |
 * | cpu    |      - |      1024 |        - | Previous test 20260127 was over 2 hours |
 * | qpu    |      1 |         " | 2842.475 | Accidentally ran 2 instances of gravity |
 * |        |     16 |         " |  205.503 |                                         |
 * |--------|--------|-----------|----------|-----------------------------------------|
 */
#include "settings.h"
#include "animate.h"
#include "model.h"
#include "galaxies.h"
#include "scalar.h"
#include "kernel.h"
#include "Support/Helpers.h"  // to_file()
#include "vc4/RegisterMap.h"
#include "LibSettings.h"
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


/**
 * @brief Run the scalar kernel
 */
void run_scalar_kernel() {
  warn << "Run scalar kernel";

  Image img(512, 512);
  img.set_conversion_factor(IMG_CONVERSION_FACTOR);

  Timer timer("Scalar timer", true);

  init_orbital_entities();
  scalar_run(img);

  // The plot image is always filled in, output is optional.
  if (settings.output_orbits) {
    img.save("gravity.bmp");
  }
}


/**
 * Galaxies does not use the same physics constants as the other model.
 * For this reason, the loop goes over number of years, not seconds per day.
 */
void galaxy_run(Image &img) {
  timers.start("cpu galaxy");

  for (int j = 0; j < settings.years; ++j) {
    galaxies::step();

    galaxies::draw(img);                 // Update orbits image
    save_animate(galaxies::draw, true);  // 2nd param: always save, ignore animate skip
  }

  timers.stop("cpu galaxy");
}


void run_galaxy() {
  warn << "Run galaxy";

  Image img(512, 512);
  img.set_conversion_factor(1000.0);
  galaxies::determine_factor(img);

  Timer timer("Galaxy timer", true);

  galaxy_run(img);

  // The plot image is always filled in, output is optional.
  if (settings.output_orbits) {
    img.save("gravity.bmp");
  }
}


/**
 * @brief Run the gpu kernel
 */
void run_gpu_kernel() {
  warn << "Run GPU kernel" 
       << ", num qpu's: " << settings.num_qpus;

  Image img(512, 512);
  img.set_conversion_factor(IMG_CONVERSION_FACTOR);

  init_orbital_entities();
  Model m;  // Orbit image
  m.init();

  auto plot_image = [&m] (Image &img) {
    for (int i = 0; i < (int) m.x.size(); ++i) {
       img.plot(m.x[i], m.y[i]);
     }
  };

  plot_image(m.orbit_img);        // Init the orbit image

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

      plot_image(m.orbit_img);        // Update the orbit image - always done
      save_animate(plot_image);
      t += batch_steps()*dt;
    }

    m.save_orbits();                  // Save orbit image if specified
    save_animate(plot_image, true);
    timers.stop("QPU gravity");
  }
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

  if (settings.kernel == 1) {
    if (settings.model == "galaxy") {
      run_galaxy();
    } else {
      run_scalar_kernel();
    }
  } else {
    run_gpu_kernel();
  }

  timers.end(true);
  return 0;
}
