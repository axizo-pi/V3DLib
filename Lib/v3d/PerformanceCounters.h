#ifndef _LIB_V3D_PERFORMANCECOUNTERS_H
#define _LIB_V3D_PERFORMANCECOUNTERS_H

#ifdef QPU_MODE

#include <vector>
#include <string>
#include <stdint.h>

namespace V3DLib {
namespace v3d {

/**
 * @brief Performance counters for `v3d`
 *
 * ----------------------------------------------------------------------------
 * # Setup
 *
 * * There are an unknown number of counters (at least 33, at most 127) per core.
 *   Each core has its own set, the counters are the same per core.
 *
 * * In order to use a counter, it must be allocated to a 'source register'.
 *   There are 32 of these per core, and that's the maximum number of counters that you can
 *   access in one go.
 *
 * * To allocate a counter, a bit is set in register `CORE_PCTR_0_EN` to specify the source register.
 *   The counter to use in the selected source register is specified in the corresponding bit field
 *   in registers CORE_PCTR_0_SRC_0 to CORE_PCTR_0_SRC_7. Each of these registers contains four bitfields,
 *   for four consecutive source registers. The bitfields are 6 bits wide, this places an upper limit on
 *   the number of available counters.
 *
 * * Access to a counter is stopped by setting the corresponding source register bit in CORE_PCTR_0_EN to zero.
 *
 *
 * # Further notes
 *
 * * The v3d has one single core (`vc4` is organized differently and has no cores).
 *   This class thus takes one single core into account. Should more cores be added in future versions,
 *   the class needs to be adjusted.
 *
 * * The setup is comparable to the way the performance counters work on `vc4`. Differences are:
 *   - `v3d` has 32 source registers per core, `vc4` 16 global registers
 *   - in `v3d` bitfields are used to specify counter per source register, `vc4` has no bitfields
 *
 *
 * # Source 
 *
 * Taken from `py-videocore6` project, which is all the informaction available on counters.
 * Source: https://github.com/Idein/py-videocore6/blob/58bbcb88979c8ee6c8bd847da884c2405994432b/videocore6/v3d.py#L241
 */
class PerformanceCounters {
public:
  enum {
    NUM_SRC_REGS      = 32,
    NUM_PERF_COUNTERS = 88,      // No idea how many there are, this is an assumption based on vc4pc
  };


  /**
   * @brief Enums for vc6 performance counters.
   *
   * These are in fact deprecated and only apply to vc6.
   * Taken from /usr/include/drm/v3d_drm.h on debian 12.
   */
  enum vc6pc {
    V3D_PERFCNT_FEP_VALID_PRIMTS_NO_PIXELS,
    V3D_PERFCNT_FEP_VALID_PRIMS,
    V3D_PERFCNT_FEP_EZ_NFCLIP_QUADS,
    V3D_PERFCNT_FEP_VALID_QUADS,
    V3D_PERFCNT_TLB_QUADS_STENCIL_FAIL,
    V3D_PERFCNT_TLB_QUADS_STENCILZ_FAIL,
    V3D_PERFCNT_TLB_QUADS_STENCILZ_PASS,
    V3D_PERFCNT_TLB_QUADS_ZERO_COV,
    V3D_PERFCNT_TLB_QUADS_NONZERO_COV,
    V3D_PERFCNT_TLB_QUADS_WRITTEN,
    V3D_PERFCNT_PTB_PRIM_VIEWPOINT_DISCARD,  // 10
    V3D_PERFCNT_PTB_PRIM_CLIP,
    V3D_PERFCNT_PTB_PRIM_REV,
    V3D_PERFCNT_QPU_IDLE_CYCLES,
    V3D_PERFCNT_QPU_ACTIVE_CYCLES_VERTEX_COORD_USER,
    V3D_PERFCNT_QPU_ACTIVE_CYCLES_FRAG,
    V3D_PERFCNT_QPU_CYCLES_VALID_INSTR,     // 16
    V3D_PERFCNT_QPU_CYCLES_TMU_STALL,
    V3D_PERFCNT_QPU_CYCLES_SCOREBOARD_STALL,
    V3D_PERFCNT_QPU_CYCLES_VARYINGS_STALL,
    V3D_PERFCNT_QPU_IC_HIT,                 // 20
    V3D_PERFCNT_QPU_IC_MISS,
    V3D_PERFCNT_QPU_UC_HIT,
    V3D_PERFCNT_QPU_UC_MISS,
    V3D_PERFCNT_TMU_TCACHE_ACCESS,
    V3D_PERFCNT_TMU_TCACHE_MISS,
    V3D_PERFCNT_VPM_VDW_STALL,
    V3D_PERFCNT_VPM_VCD_STALL,
    V3D_PERFCNT_BIN_ACTIVE,
    V3D_PERFCNT_RDR_ACTIVE,
    V3D_PERFCNT_L2T_HITS,                   // 30
    V3D_PERFCNT_L2T_MISSES,

    // Assumption: the number of clock cycles that a program ran.
    // The number is variable per run, but always in the same range.
    // getting value twice show monotonic increasing values
    V3D_PERFCNT_CYCLE_COUNT,

    V3D_PERFCNT_QPU_CYCLES_STALLED_VERTEX_COORD_USER,
    V3D_PERFCNT_QPU_CYCLES_STALLED_FRAGMENT,
    V3D_PERFCNT_PTB_PRIMS_BINNED,
    V3D_PERFCNT_AXI_WRITES_WATCH_0,
    V3D_PERFCNT_AXI_READS_WATCH_0,
    V3D_PERFCNT_AXI_WRITE_STALLS_WATCH_0,
    V3D_PERFCNT_AXI_READ_STALLS_WATCH_0,
    V3D_PERFCNT_AXI_WRITE_BYTES_WATCH_0,
    V3D_PERFCNT_AXI_READ_BYTES_WATCH_0,
    V3D_PERFCNT_AXI_WRITES_WATCH_1,
    V3D_PERFCNT_AXI_READS_WATCH_1,
    V3D_PERFCNT_AXI_WRITE_STALLS_WATCH_1,
    V3D_PERFCNT_AXI_READ_STALLS_WATCH_1,
    V3D_PERFCNT_AXI_WRITE_BYTES_WATCH_1,
    V3D_PERFCNT_AXI_READ_BYTES_WATCH_1,
    V3D_PERFCNT_TLB_PARTIAL_QUADS,
    V3D_PERFCNT_TMU_CONFIG_ACCESSES,
    V3D_PERFCNT_L2T_NO_ID_STALL,
    V3D_PERFCNT_L2T_COM_QUE_STALL,
    V3D_PERFCNT_L2T_TMU_WRITES,
    V3D_PERFCNT_TMU_ACTIVE_CYCLES,
    V3D_PERFCNT_TMU_STALLED_CYCLES,
    V3D_PERFCNT_CLE_ACTIVE,
    V3D_PERFCNT_L2T_TMU_READS,
    V3D_PERFCNT_L2T_CLE_READS,
    V3D_PERFCNT_L2T_VCD_READS,
    V3D_PERFCNT_L2T_TMUCFG_READS,
    V3D_PERFCNT_L2T_SLC0_READS,
    V3D_PERFCNT_L2T_SLC1_READS,
    V3D_PERFCNT_L2T_SLC2_READS,
    V3D_PERFCNT_L2T_TMU_W_MISSES,
    V3D_PERFCNT_L2T_TMU_R_MISSES,
    V3D_PERFCNT_L2T_CLE_MISSES,
    V3D_PERFCNT_L2T_VCD_MISSES,
    V3D_PERFCNT_L2T_TMUCFG_MISSES,
    V3D_PERFCNT_L2T_SLC0_MISSES,
    V3D_PERFCNT_L2T_SLC1_MISSES,
    V3D_PERFCNT_L2T_SLC2_MISSES,
    V3D_PERFCNT_CORE_MEM_WRITES,
    V3D_PERFCNT_L2T_MEM_WRITES,
    V3D_PERFCNT_PTB_MEM_WRITES,
    V3D_PERFCNT_TLB_MEM_WRITES,
    V3D_PERFCNT_CORE_MEM_READS,
    V3D_PERFCNT_L2T_MEM_READS,
    V3D_PERFCNT_PTB_MEM_READS,
    V3D_PERFCNT_PSE_MEM_READS,
    V3D_PERFCNT_TLB_MEM_READS,
    V3D_PERFCNT_GMP_MEM_READS,
    V3D_PERFCNT_PTB_W_MEM_WORDS,
    V3D_PERFCNT_TLB_W_MEM_WORDS,
    V3D_PERFCNT_PSE_R_MEM_WORDS,
    V3D_PERFCNT_TLB_R_MEM_WORDS,
    V3D_PERFCNT_TMU_MRU_HITS,
    V3D_PERFCNT_COMPUTE_ACTIVE,
    V3D_PERFCNT_NUM,
  };


  static void enter(std::vector<int> srcs);
  static std::string showEnabled();

private:
  static const char *Description[NUM_PERF_COUNTERS];

  static void exit();
};

}  // namespace v3d
}  // namespace V3DLib

#endif  // QPU_MODE

#endif  // _LIB_V3D_PERFORMANCECOUNTERS_H
