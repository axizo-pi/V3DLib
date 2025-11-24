#ifndef _V3DLIB_BASEKERNEL_H_
#define _V3DLIB_BASEKERNEL_H_
#include <memory>
#include "KernelDriver.h"
#include "Support/BaseSettings.h"

namespace V3DLib {


/**
 * -------------------------
 * NOTES
 * =====
 *
 * 1. A kernel can be invoked with the following methods of `Kernel`:
 *
 *     - interpret(...)  - run on source code interpreter
 *     - emu(...)        - run on the target code emulator (`vc4` code only)
 *     - qpu(...)        - run on physical QPUs (when QPU_MODE enabled, run emu()))
 *     - run(...)        - Call the best run option from previous depending on settings and compile options
 *
 *    The interpreter and emulator are useful for:
 *      - cross-platform compatibility
 *      - development/debugging
 *      - equivalence testing for the hardware QPU
 *
 *    Keep in mind that the interpreter and emulator are really slow
 *    in comparison to the hardware.
 *
 *
 * 2. The interpreter and emulator will run on any architecture.
 *
 *    The compile-time option `-D QPU_MODE` enables the execution of code on
 *    the physical VideoCore. This will work only on the Rasberry Pi.
 *    This define is disabled in the makefile for non-Pi platforms.
 *
 *    There are three possible shared memory allocation schemes:
 *
 *    1. Using main memory - selectable in code, will not work for hardware GPU
 *    2. `vc4` memory allocation - used when communicating with hardware GPU on
 *       all Pi's prior to Pi4
 *    3. `v3d` memory allocation - used when communicating with hardware GPU on the Pi4 
 *
 *    The memory pool implementations are in source files
 *    called `BufferObject.cpp` (Under `Target`, `v3d` and `vc4`).
 *
 *
 * 3. The code generation for v3d and vc4 has diverged, even at the level of
 *    source code. To handle this, the code generation for both cases is 
 *    isolated in 'kernel drivers'. This encapsulates the differences
 *    for the kernel.
 *
 *    Because the interpreter and emulator work with vc4 code,
 *    the vc4 kernel driver is always used, even if only assembling for v3d.
 */
class BaseKernel {
public:
  BaseKernel(BaseSettings const &settings);
  BaseKernel(BaseKernel &&k) = default;

  bool has_driver() const;
  V3DLib::KernelDriver &driver();
  V3DLib::KernelDriver const &driver() const;

  void compile_init(bool do_vc4);
  void pretty(const char *filename = nullptr, bool output_qpu_code = true);

  BaseKernel &setNumQPUs(int n) { m_settings.num_qpus = n; return *this; }
  int numQPUs() const { return m_settings.num_qpus; }

  void run();

  void emu();
  void interpret();
  void qpu();

  Code const &code() const { return m_driver->code(); }
  IntList const &params() const { return uniforms; }  // Can't name it uniforms because the data member is called that

  std::string compile_info() const;
  void dump_compile_data(char const *filename);
  bool has_errors() const;
  std::string get_errors() const;
  std::string info() const;

protected:
  BaseSettings m_settings;
  IntList uniforms;                // Parameters to be passed to kernel


  // Defined as unique pointer so that is easily survives std::move
  // (There are other reasons but this is the main one)
  std::unique_ptr<KernelDriver> m_driver;
};


}  // namespace V3DLib

#endif  // _V3DLIB_BASEKERNEL_H_
