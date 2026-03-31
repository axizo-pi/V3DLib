#ifdef QPU_MODE

#ifndef _V3DLIB_VC4_REGISTERMAP_H
#define _V3DLIB_VC4_REGISTERMAP_H
#include <stdint.h>
#include <string>

namespace V3DLib {

/**	
 * Needs to be public, used in PerformanceCounters.
 */
enum Index {
  V3D_BASE    = (0xc00000 >> 2),
  V3D_IDENT0  = 0,
  V3D_IDENT1,
  V3D_IDENT2,
  V3D_L2CACTL = (0x00020 >> 2),
  V3D_SQRSV0  = (0x00410 >> 2 ), // Scheduler Register QPUS 0-7
  V3D_SQRSV1,                    // Scheduler Register QPUS 8-15

  V3D_CT0CS   = (0x00100 >> 2),  // Control List Executor Thread 0 Control and Status.
  V3D_CT1CS,                     // Control List Executor Thread 0 Control and Status.

  V3D_SQCNTL  = (0x00418 >> 2),  // QPU Scheduler Control

  // Program Request registers
  V3D_SRQPC   = (0x00430 >> 2),   // QPU User Program Request Program Address
  V3D_SRQUA,                      // QPU User Program Request Uniforms Address
  V3D_SRQUL,                      // QPU User Program Request Uniforms Length
  V3D_SRQCS,                      // QPU User Program Request Control and Status

  //
  // Performance counter register slots.
  //
  // There are 30 performance counters, but only 16 registers available
  // to access them. You therefore have to map the counters you are
  // interested in to an available slot.
  //
  // There is actually no use in specifying all names fully, except to
  // have the list complete. Internally,  the counter registers are taken as offsets
  // from the first (V3D_PCTR0 and V3D_PCTRS0), so for the list it's enough to specify
  // only that one and set an enum for the end of the list.
  // TODO: perhaps make this so
  //
  // PC: 'Performance Counter' below
  //
  V3D_PCTRC = (0x00670 >> 2),  // PC Clear     - write only
  V3D_PCTRE,                   // PC Enables   - read/write
  V3D_PCTR0 = (0x00680 >> 2),  // PC Count 0   - read/write
  V3D_PCTRS0,                  // PC Mapping 0 - read/write
  V3D_PCTR1,                   // PC Count 1
  V3D_PCTRS1,                  // PC Mapping 1
  V3D_PCTR2,                   // etc.
  V3D_PCTRS2,
  V3D_PCTR3,
  V3D_PCTRS3,
  V3D_PCTR4,
  V3D_PCTRS4,
  V3D_PCTR5,
  V3D_PCTRS5,
  V3D_PCTR6,
  V3D_PCTRS6,
  V3D_PCTR7,
  V3D_PCTRS7,
  V3D_PCTR8,
  V3D_PCTRS8,
  V3D_PCTR9,
  V3D_PCTRS9,
  V3D_PCTR10,
  V3D_PCTRS10,
  V3D_PCTR11,
  V3D_PCTRS11,
  V3D_PCTR12,
  V3D_PCTRS12,
  V3D_PCTR13,
  V3D_PCTRS13,
  V3D_PCTR14,
  V3D_PCTRS14,
  V3D_PCTR15,
  V3D_PCTRS15
};


const int MAX_AVAILABLE_QPUS = 16;

/**
 * @brief Data structure for returning scheduler register values.
 *
 * Length is max available slots, not the actual number of QPU's.
 */
struct SchedulerRegisterValues {
  int  qpu[MAX_AVAILABLE_QPUS];
  bool set_qpu[MAX_AVAILABLE_QPUS];  // Used for write option

  SchedulerRegisterValues() {
    for (int i = 0; i < MAX_AVAILABLE_QPUS; ++i) {
      set_qpu[i] = false;
    }
  }
};


/**
 * @brief interface for the vc4 registers.
 *
 * This implementation is far from complete.
 * The reading of more registers can be added as needed.
 */
namespace RegisterMap {

uint32_t readRegister(int offset);
void writeRegister(int offset, uint32_t value);

int numSlices();
int numQPUPerSlice();
int numTMUPerSlice();
int VPMMemorySize();
int L2CacheEnabled();
void L2Cache_enable(bool enable);

SchedulerRegisterValues SchedulerRegisters();
void SchedulerRegisters(SchedulerRegisterValues values);
void resetAllSchedulerRegisters();

int TechnologyVersion();
bool checkThreadErrors();

std::string ProgramRequestStatus();

} // RegisterMap

}  // namespace V3DLib

#endif  // _V3DLIB_VC4_REGISTERMAP_H
#endif  // QPU_MODE
