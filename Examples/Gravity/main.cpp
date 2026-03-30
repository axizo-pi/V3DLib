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
  // Disable the cache - vc4 only
  if (Platform::compiling_for_vc4()) {
    // Disable L2 cache: this ensure that DMA and TMU can work together
    RegisterMap::L2Cache_enable(false);
    warn << "L2CacheEnabled(): " << RegisterMap::L2CacheEnabled();
  }

  // Disable TMU load - does nothing for vc7
  LibSettings::tmu_load tmu(false);
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
 * I remember that adding barrier made a massive difference; now it doesn't do anything
 * for _any_ pi.
 *
 * Runs below done with BATCH_STEPS = 1; this **might make a big difference** for `vc`.
 * 
 * Examination with `vc7` show that using `barrier` with `BATCH_STEPS = 400` makes a
 * significant difference in the output. Conclusion is that `barrier` is required in this case.
 *
 * **TODO** examine this for `vc6`, `vc4`.
 *
 * ## Pi2, vc4
 *
 * | barrier | QPU's | TMU | L2 Cache | Output OK | time | mbox error | Comment |
 * | ------- | ----- | --- | -------- | --------- | ---- | ---------- | ------- |
 * | yes     |  1    | yes | disabled | yes       | 42s  | no,  5     | Lost power at 6            |
 * | yes     |  2    | yes | disabled | yes       | 34s  | no,  3     | |
 * | yes     |  6    | yes | disabled | ?         | -    | YES, 1     | mbox error consistent at 1 |
 * | yes     | 12    | yes | disabled | ?         | -    | YES, 1     | mbox error consistent at 1 |
 * | no      |  1    | yes | disabled | yes       | 42s  | no,  3     | |
 * | no      |  2    | yes | disabled | yes       | 32s  | no,  3     | |
 * | no      |  3    | yes | disabled | yes       | 27s  | no,  3     | faster than n=12!?         |
 * | no      |  6    | yes | disabled | yes       | 26s  | no,  3     | idem |
 * | no      |  9    | yes | disabled | yes       | 27s  | no,  4     | idem |
 * | no      | 12    | yes | disabled | yes       | 29s  | no,  3     | |
 * | no      |  1    | no  | disabled | yes       | 41s  | no,  2     | |
 * | no      |  6    | no  | disabled | yes       | 26s  | no,  2     | faster than n=12!?         |
 * | no      | 12    | no  | disabled | yes       | 28s  | no,  3     | |
 *
 * ### Conclusions
 *
 * - The power socket of the pi2-1 appears to be sketchy; power lost during testing.
 *   The connection is not firm.
 * - barrier has no effect; it leads to mbox errors for n >= 6
 * - Less QPU's works better!
 *
 * ## Pi3, vc4
 *
 * | barrier | QPU's | TMU | L2 Cache | Output OK | time | mbox error | Comment |
 * | ------- | ----- | --- | -------- | --------- | ---- | ---------- | ------- |
 * | yes     |  1    | yes | enabled  | yes       | 56s  | no,  5     |         |
 * | "       | 12    | "   | "        | "         | 50s  | yes, 4     |         |
 * | "       |  1    | "   | disabled | "         | 36s  | no,  5     |         |
 * | "       | 12    | "   | disabled | "         | 35s  | no,  5     |         |
 * | "       |  1    | no  | disabled | NO        | 65s  | no,  1     |         |
 * | "       | 12    | no  | disabled | NO        | -    | yes, 1     | mbox error consistent at 1 |
 * | no      |  1    | "   | enabled  | yes       | 56s  | no,  5     |         |
 * | "       | 12    | "   | "        | "         | 34s  | no,  5     |         |
 * | "       |  1    | "   | disabled | "         | 36s  | no,  5     |         |
 * | "       | 12    | "   | disabled | "         | 19s  | no,  5     |         |
 * | "       |  1    | no  | enabled  | NO        | 83s  | no,  2     |         |
 * | "       | 12    | no  | enabled  | NO        | 62s  | no,  2     |         |
 * | "       |  1    | no  | disabled | NO        | 62s  | no,  2     |         |
 * | "       | 12    | no  | disabled | NO        | 65s  | no,  2     |         |
 *
 * ### Conclusions
 *
 * - DMA load does _not_ give good results
 * - Disabling L2 cache with TMU load actually _increases_ performance
 * - barrier has no effect; it can only decrease performance
 *
 * ## Pi4, vc6
 *
 * | barrier | QPU's | TMU | L2 Cache | Ouitput OK | time | mbox error | Comment |
 * | ------- | ----- | --- | -------- | --------- | ---- | ---------- | ------- |
 * | yes     |  1    | -   | -        | yes       | 20s  | no,  3     |         |
 * | yes     | 16    | -   | -        | yes       | 13s  | no,  3     |         |
 * | no      |  1    | -   | -        | yes       | 20s  | no,  3     |         |
 * | no      |  8    | -   | -        | yes       | 13s  | no,  3     |         |
 *
 * ### Conclusions
 *
 * - `barrier` has no effect
 *
 * ## Pi5, vc7
 *
 * | barrier | QPU's | TMU | L2 Cache | Ouput OK | time | mbox error | Comment |
 * | ------- | ----- | --- | -------- | -------- | ---- | ---------- | ------- |
 * | yes     |  1    | -   | -        | yes      | 9s   | no,  3     |         |
 * | yes     | 16    | -   | -        | yes      | 4s   | no,  5     |         |
 * | no      |  1    | -   | -        | yes      | 9s   | no,  5     |         |
 * | no      | 16    | -   | -        | yes      | 4s   | no,  5     |         |
 *
 * ### Conclusions
 *
 * - `barrier` has no effect (WTF)
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
