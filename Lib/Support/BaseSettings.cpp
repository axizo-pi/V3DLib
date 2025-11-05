#include "BaseSettings.h"
#include "Kernel.h"
#include "debug.h"

#ifdef QPU_MODE
#include "Support/Platform.h"
#include "vc4/PerformanceCounters.h"
#include "v3d/PerformanceCounters.h"
#endif  // QPU_MODE

namespace V3DLib {

void BaseSettings::process(BaseKernel &k) {
  // NOTE: For multiple calls here (entirely possible, HeatMap does this),
  //       this will prevent dumping the v3d code (mnemonics, actually) on every call.
  if (output_code) {
    if (output_count == 0) {
      assert(!name.empty());
      std::string code_filename = name + "_code.txt";

      k.pretty(code_filename.c_str());
    } else if (output_count == 1) {
      warning("Not outputting code more than once");
    }

    output_count++;
  }
}


/**
 * @brief Performance Counters: Enable the counters we are interested in
 */
void BaseSettings::startPerfCounters() {

#ifdef QPU_MODE
  if (!show_perf_counters) return;

  using PC = V3DLib::vc4::PerformanceCounters;
 
  printf("TODO Settings::startPerfCounters: add vc7\n");

  if (Platform::run_vc4()) {
    PC::enable({
      PC::QPU_INSTRUCTIONS,
      PC::QPU_STALLED_TMU,
      PC::L2C_CACHE_HITS,
      PC::L2C_CACHE_MISSES,
      PC::QPU_INSTRUCTION_CACHE_HITS,
      PC::QPU_INSTRUCTION_CACHE_MISSES,
      PC::QPU_CACHE_HITS,
      PC::QPU_CACHE_MISSES,
      PC::QPU_IDLE,
    });
  } else {
    using PC3 = V3DLib::v3d::PerformanceCounters;

    PC3::enter({
      // vc4 counters, check if same and working.
      // They work, but overlap is hard to detect with vc4.
      PC::QPU_INSTRUCTIONS,
      PC::QPU_STALLED_TMU,
      PC::L2C_CACHE_HITS,
      PC::L2C_CACHE_MISSES,
      PC::QPU_INSTRUCTION_CACHE_HITS,
      PC::QPU_INSTRUCTION_CACHE_MISSES,
      PC::QPU_CACHE_HITS,
      PC::QPU_CACHE_MISSES,
      PC::QPU_IDLE,

      PC3::CORE_PCTR_CYCLE_COUNT,  // specific for v3d
      // CHECKED for <= 40
    });
  }
#endif
}


void BaseSettings::stopPerfCounters() {
#ifdef QPU_MODE
  if (!show_perf_counters) return;
 
  std::string output;

  printf("TODO Settings::stopPerfCounters: add vc7\n");

  if (Platform::run_vc4()) {
    // Show values current counters
    using PC = V3DLib::vc4::PerformanceCounters;

    output = PC::showEnabled();
  } else {
    using PC = V3DLib::v3d::PerformanceCounters;
    output = PC::showEnabled();
  }

  printf("%s\n", output.c_str());
#endif
}


} // namespace V3DLib
