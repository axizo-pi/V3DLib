#ifdef QPU_MODE

#include "RegisterMap.h"
#include "Support/Platform.h"
#include "Support/basics.h"   // fatal()
#include "Support/Helpers.h"  // indentBy()
#include "Mailbox.h"          // mapmem()
#include "vc4.h"
#include <bcm_host.h>
#include <cstring>
#include <memory>

namespace V3DLib {
namespace {

/**
 * @brief Masks for scheduler registers
 *
 * **UNUSED**
 */
enum SchedulerMasks : int {
  DO_NOT_USE_FOR_USER_PROGRAMS = 1,
  DO_NOT_USE_FOR_FRAGMENT_SHADERS = 2,
  DO_NOT_USE_FOR_VERTEX_SHADERS = 4,
  DO_NOT_USE_FOR_COORDINATE = 8
};


enum ErrStatFields {
  VPAEABB = 0,
  VPAERGS,
  VPAEBRGL,
  VPAERRGL,
  VPMEWR,
  VPMERR,
  VPMERNA,
  VPMEWNA,
  VPMEFNA,
  VPMEAS,
  VDWE,
  VCDE,
  VCDI, 
  VCMRE,
  VCMBE,
  L2CARE, // 15

  MAX = L2CARE
};


/**
 * @brief Get labels for error status registers
 *
 * ========================================
 *
 * Notes
 * -----
 * - TMU - Texture and Memory Lookup Unit
 * - VPM - Vertex Pipe Memory
 * - VCM - Vertex Cache Manager, 'General DMA Read Data'.
 *         So, the 'read from main mem' option of the VPM.
 */
std::string errStatLabel(ErrStatFields val) {
  switch (val) {
    case VPAEABB:  return "VPM Allocator error - allocating base while busy";
    case VPAERGS:  return "VPM Allocator error - request too big";
    case VPAEBRGL: return "VPM Allocator error - binner request greater than limit";
    case VPAERRGL: return "VPM Allocator error - renderer request greater than limit";
    case VPMEWR:   return "VPM error - write range";
    case VPMERR:   return "VPM error - read range";
    case VPMERNA:  return "VPM error - read non-allocated";
    case VPMEWNA:  return "VPM error - write non-allocated";
    case VPMEFNA:  return "VPM error - free non-allocated";
    case VPMEAS:   return "VPM error - allocated size error";
    case VDWE:     return "VDW error - address overflows";
    case VCDE:     return "VCD error - FIFO pointers out of sync";
    case VCDI:     return "VCD Idle";
    case VCMRE:    return "VCM error (renderer)";
    case VCMBE:    return "VCM error (binner)";
    case L2CARE:   return "L2C AXI Receive Fifo Overrun error";
    default: assert(false);
  }
}


/**
 *
 * Implemented as singleton with lazy load, so that it's 
 * not initialized when it's not used.
 */
class RegisterMapClass {
public:  
  RegisterMapClass(RegisterMapClass const &) = delete;
  void operator=(RegisterMapClass const &) = delete;

  RegisterMapClass();
  ~RegisterMapClass();

  uint32_t read(int offset) const;
  void write(int offset, uint32_t value);

  volatile uint32_t *m_addr{nullptr};
  unsigned m_size{0};

  static void check_page_align(unsigned addr);
};


RegisterMapClass::RegisterMapClass() {
  enableQPUs();

  bcm_host_init();
  unsigned addr = bcm_host_get_peripheral_address();

  m_size = bcm_host_get_peripheral_size();

  check_page_align(addr);

  // Following succeeds if it returns.
  m_addr = (uint32_t *) mapmem(addr, m_size);
  assert(m_addr != nullptr);

  bcm_host_deinit();
}


RegisterMapClass::~RegisterMapClass() {
  unmapmem((void *) m_addr, m_size);
  disableQPUs();
}


/**
 * @brief Get the 32-bit value of the register at the given offset in the map
 */
uint32_t RegisterMapClass::read(int offset) const {
  return m_addr[V3D_BASE + offset];
}


/**
 * @brief Set the 32-bit value of the register at the given offset in the map
 */
void RegisterMapClass::write(int offset, uint32_t value) {
  m_addr[V3D_BASE + offset] = value;
}


void RegisterMapClass::check_page_align(unsigned addr) {
  long pagesize = sysconf(_SC_PAGESIZE);

  if (pagesize <= 0) {
    char buf[64];
    sprintf(buf, "error: sysconf: %s", strerror(errno));
    fatal(buf);
  }

  if (addr & (((unsigned) pagesize) - 1)) {
    fatal("error: peripheral address is not aligned to page size");
  }
}


std::unique_ptr<RegisterMapClass> _instance;

/**
 * @brief Masks for bitfields of register V3D_L2CACTL
 */
enum V3D_L2CACTL_bits {
  L2CCLR = 0x4,  // L2 Cache Clear, Write ‘1’ to clear the L2 Cache     (write only)
  L2CDIS = 0x2,  // L2 Cache Disable, Write ‘1’ to disable the L2 Cache (write only)
  L2CENA = 0x1,  // L2 Cache Enable, Reads state of cache enable bit.   (read/write)
                 // Write ‘1’ to enable the L2 Cache (write of ‘0’ has no effect)
};


RegisterMapClass &instance() {
  if (!Platform::run_vc4()) {
    cerr << "RegisterMap can only be accessed on a platform having VideoCore4" << thrw;
  }

  if (_instance.get() == nullptr) {
    _instance.reset(new RegisterMapClass);
  }

  return *_instance.get();
}


void checkVersionString(uint32_t  reg) {
  const char *ident = "V3D";
  char buf[4];

  buf[0] = reg & 0xff;
  buf[1] = (reg >> 8)  & 0xff;
  buf[2] = (reg >> 16)  & 0xff;
  buf[3] = '\0';
  
  if (strncmp(ident, buf, 3)) {
    printf("Id string is not the expected 'V3D' but '%s'!\n", buf);
  } else {
    //printf("Id string checks out\n");
  }
}

}  // anon namespace


namespace RegisterMap {

int numSlices() {
  uint32_t reg = readRegister(V3D_IDENT1);
  return (reg >> 4) & 0xf;
}


int numQPUPerSlice() {
  return (readRegister(V3D_IDENT1) >> 8) & 0xf;
}


int numTMUPerSlice() {
  return (readRegister(V3D_IDENT1) >> 12) & 0xf;
}


/**
 * @brief get the size of the VPM.
 *
 * @return Size of VPM in KB
 */
int VPMMemorySize() {
  uint32_t reg = readRegister(V3D_IDENT1);
  int value = (reg >> 28) & 0xf;

  if (value == 0) return 16;  // According to reference doc p.97
  return value;
}


int L2CacheEnabled() {
  return (readRegister(V3D_L2CACTL) & L2CENA);
}


/**
 * @brief Enable or disable the L2 cache.
 *
 * @param enable  if true, cache is enabled, if false cache is disabled.
 *
 * ----------------------
 * 
 * Notes
 * =====
 *
 * - A reason to disable the cache, is when `VPM`(DMA) write are combined with
 *   with `TMU` writes on the same buffer object. This is unavoidable on `vc4`.  
 *   **The `DMA` sidesteps the L2 cache**. The L2 cache is not refreshed after
 *   a `DMA` write.
 *   
 *   This happened with program `Gravity`, which stores acceleration values
 *   in a buffer object within the same kernel execution.
 *   
 *   This is not an issue for `v3d`, which uses `TMU` exclusively.
 *
 * - Thankfully, the performance of disabling the cache is not severe
 *   (At least, for `Gravity`).
 *
 * - Also thankfully, the L2 cache is enabled automatically between runs
 *   of programs calling kernels.
 */
void L2Cache_enable(bool enable) {
  if (enable) {
    writeRegister(V3D_L2CACTL, L2CENA);
  } else {
    writeRegister(V3D_L2CACTL, L2CDIS);
  }
}


/**
 * @brief Read the scheduler register values for all QPU's.
 *
 * This reads both scheduler registers.
 *
 * *NOTE:*  The scheduler registers are read/write
 *
 * @return struct with 'do not use' values for all possible values
 */
SchedulerRegisterValues SchedulerRegisters() {
  SchedulerRegisterValues ret;

  uint32_t reg0 = readRegister(V3D_SQRSV0);
  uint32_t reg1 = readRegister(V3D_SQRSV1);

  ret.qpu[ 0] = reg0       & 0x0f;
  ret.qpu[ 1] = reg0 >>  4 & 0x0f;
  ret.qpu[ 2] = reg0 >>  8 & 0x0f;
  ret.qpu[ 3] = reg0 >> 12 & 0x0f;
  ret.qpu[ 4] = reg0 >> 16 & 0x0f;
  ret.qpu[ 5] = reg0 >> 20 & 0x0f;
  ret.qpu[ 6] = reg0 >> 24 & 0x0f;
  ret.qpu[ 7] = reg0 >> 28 & 0x0f;

  ret.qpu[ 8] = reg1       & 0x0f;
  ret.qpu[ 9] = reg1 >>  4 & 0x0f;
  ret.qpu[10] = reg1 >>  8 & 0x0f;
  ret.qpu[11] = reg1 >> 12 & 0x0f;
  ret.qpu[12] = reg1 >> 16 & 0x0f;
  ret.qpu[13] = reg1 >> 20 & 0x0f;
  ret.qpu[14] = reg1 >> 24 & 0x0f;
  ret.qpu[16] = reg1 >> 28 & 0x0f;

  return ret;
}


/**
 * @brief Write scheduler register values for selected QPU's.
 *
 * This sets values in both scheduler registers.
 *
 * **NOTE:**  The scheduler registers are read/write
 *
 * **TODO:** Not tested yet!
 */
void SchedulerRegisters(SchedulerRegisterValues values) {
  uint32_t reg0 = readRegister(V3D_SQRSV0);
  uint32_t reg1 = readRegister(V3D_SQRSV1);
  uint32_t new_reg0 = reg0;
  uint32_t new_reg1 = reg1;

  for (int i = 0; i < MAX_AVAILABLE_QPUS/2; ++i) {
    int offset = 4*i;

    if (values.set_qpu[i]) {
      new_reg0 = (0x0f & values.qpu[i]) << offset;
    }
  }

  for (int i = MAX_AVAILABLE_QPUS/2; i < MAX_AVAILABLE_QPUS; ++i) {
    int offset = 4*i - MAX_AVAILABLE_QPUS/2;

    if (values.set_qpu[i]) {
      new_reg1 = (0x0f & values.qpu[i]) << offset;
    }
  }

  if (reg0 != new_reg0) {  // Value reg0 changed
    writeRegister(V3D_SQRSV0, new_reg0);
  }

  if (reg1 != new_reg1) {  // Value reg1 changed
    writeRegister(V3D_SQRSV1, new_reg1);
  }
}


void resetAllSchedulerRegisters() {
  SchedulerRegisterValues values;

  for (int i = 0; i < MAX_AVAILABLE_QPUS; ++i) {
    values.set_qpu[i] = true;   // write for all QPU's
    values.qpu[i]     = 0;      // allow all program types (eg 0)
  }

  SchedulerRegisters(values);
}


std::string showSchedulerRegisters() {
  std::string ret;

  int numQPUs = numSlices() * numQPUPerSlice();
  SchedulerRegisterValues values = SchedulerRegisters();

  ret << "Scheduler registers, do not use:";

  for (int i = 0; i < numQPUs; ++i ) {
    ret << "\n  QPU " << i <<  ": ";

    int val = values.qpu[i];

    if (val & DO_NOT_USE_FOR_USER_PROGRAMS) {
      ret << "user programs, ";
    }

    if (val & DO_NOT_USE_FOR_FRAGMENT_SHADERS) {
      ret << "fragment shaders, ";
    }

    if (val & DO_NOT_USE_FOR_VERTEX_SHADERS) {
      ret << "vertex shaders, ";
    }

    if (val & DO_NOT_USE_FOR_COORDINATE) {
      ret << "coordinate";
    }
  }

  ret << "\n";

  return ret;
}


/**
 * @brief Convenience function for getting a register value.
 *
 * This avoids having to use `instance()->` for every read access.
 */
uint32_t readRegister(int offset) {
  return instance().read(offset);
}


/**
 * @brief Convenience function for setting a register value.
 *
 * This avoids having to use `instance()->` for every write access.
 */
void writeRegister(int offset, uint32_t value) {
  return instance().write(offset, value);
}


int TechnologyVersion() {
  uint32_t reg = readRegister(V3D_IDENT0);

  checkVersionString(reg);

  int ret = (reg >> 24) & 0xf;
  return ret;
}


/**
 * **UNUSED**
 *
 * @return true if errors present, false otherwise
 */
bool checkThreadErrors() {
  uint32_t ERROR_BITMASK = 0x08;  // If set, control thread error

  uint32_t reg0 = readRegister(V3D_CT0CS);
  uint32_t reg1 = readRegister(V3D_CT1CS);

  bool ret = false;

  if (reg0 & ERROR_BITMASK) {
    printf("Control thread error for thread 0\n");
    ret = true;
  }

  if (reg1 & ERROR_BITMASK) {
    printf("Control thread error for thread 1\n");
    ret = true;
  }

  return ret;
}


/**
 * @param uniforms_length  number of passed uniforms.
 *        0 disables the uniforms stream.
 *        > 1023 implies an unlimited uniforms stream.
 */
void ProgramRequest(uint32_t address, uint32_t uniforms, uint32_t uniforms_length) {
  assert(uniforms_length >0 && uniforms_length < 1024);

  writeRegister(V3D_SRQUA, uniforms);
  writeRegister(V3D_SRQUL, uniforms_length);
  writeRegister(V3D_SRQPC, address);  // Assuming that this triggers the request and needs to be at end
}


std::string ProgramRequestStatus() {
  std::string ret;

  uint32_t reg = readRegister(V3D_SRQCS);

  ret << "\n"
    << "  Reg: " << reg << "\n"  // DEBUG: Is it really zero?
    << "  # Programs Completed: " << ((reg >> 16) & 0b11111111) << "\n"
    << "  # Program Requests  : " << ((reg >>  8) & 0b11111111) << "\n"
    << "  Queue Error         : " << ((reg >>  7) & 0b1)        << "\n"
    << "  Queue Length        : " << (reg & 0b111111);

  return ret;
}


/**
 * @brief Dump values of error status registers
 */
std::string ErrorStatus() {
  // Prescan for max length labels
  int max_length = 0;
  for (int i = 0; i < ErrStatFields::MAX; ++i) {
    ErrStatFields val = (ErrStatFields) i;

    int length = (int) errStatLabel(val).length();
    if (length > max_length) max_length = length;
  }

  std::string ret;

  for (int i = 0; i < ErrStatFields::MAX; ++i) {
    ErrStatFields val = (ErrStatFields) i;
    auto label = errStatLabel(val);
    auto tabs = indentBy(max_length - (int) label.length());

    ret << "  " << label << tabs << ": " << ((readRegister(V3D_ERRSTAT) >> val) & 1) << "\n";
  }

  return ret;
}

} // RegisterMap
}  // namespace V3DLib

#endif  // QPU_MODE
