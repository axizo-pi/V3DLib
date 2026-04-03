#include "BaseSettings.h"
#include "Kernel.h"
#include "debug.h"
#include "Helpers.h"

#ifdef QPU_MODE
#include "Support/Platform.h"
#include "vc4/PerformanceCounters.h"
#include "v3d/PerformanceCounters.h"
#endif  // QPU_MODE

using namespace Log;

namespace V3DLib {

void BaseSettings::dump_code(BaseKernel &k) {
  // For multiple calls here (entirely possible, HeatMap does this),
  // this will prevent dumping the v3d code (mnemonics, actually) on every call.
  if (output_code) {
    if (output_count == 0) {
      cdebug << "BaseSettings::process() outputting code";

      assert(!name.empty());
      std::string code_filename = name + "_code.txt";
      //warn << "BaseSettings::process() code_filename: " << code_filename;

      to_file(code_filename.c_str(), k.dump());
    } else if (output_count == 1) {
      warn << "Not outputting code more than once";
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
 
  // TODO add vc7\n

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
    using pc = V3DLib::v3d::PerformanceCounters::vc6pc;

    PC3::enter({
      pc::V3D_PERFCNT_QPU_CYCLES_VALID_INSTR,
      pc::V3D_PERFCNT_QPU_CYCLES_TMU_STALL,
      pc::V3D_PERFCNT_L2T_HITS,
      pc::V3D_PERFCNT_L2T_MISSES,
      pc::V3D_PERFCNT_QPU_IC_HIT,
      pc::V3D_PERFCNT_QPU_IC_MISS,
      pc::V3D_PERFCNT_QPU_UC_HIT,
      pc::V3D_PERFCNT_QPU_UC_MISS,
      pc::V3D_PERFCNT_CYCLE_COUNT,
      pc::V3D_PERFCNT_QPU_IDLE_CYCLES,
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
