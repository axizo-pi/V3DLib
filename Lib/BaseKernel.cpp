#include "BaseKernel.h"
#include "vc4/KernelDriver.h"
#include "v3d/KernelDriver.h"
#include "Support/basics.h"
#include "Source/Interpreter.h"
#include "Target/Emulator.h"

using namespace Log;

namespace V3DLib {

using ::operator<<;  // C++ weirdness

BaseKernel::BaseKernel(BaseSettings const &settings) : m_settings(settings) {}


bool BaseKernel::has_driver() const { return m_driver.get() != nullptr; }


V3DLib::KernelDriver &BaseKernel::driver() {
  assert(has_driver());
  return *m_driver;
}


V3DLib::KernelDriver const &BaseKernel::driver() const {
  assert(has_driver());
  return *m_driver;
}


void BaseKernel::compile_init() {
  //Log::info << "Called compile_init()";
  assert(m_driver.get() == nullptr);

  enum SelectKernel {
    None,
    vc4,
    v3d
  };

  SelectKernel select_kernel = None;

   //cdebug << "m_settings.run_type: " << m_settings.run_type;
   //cdebug << "Platform::compiling_for_vc4(): " << Platform::compiling_for_vc4(); 
  //cdebug << "m_settings.compile_only: " << m_settings.compile_only;

   if (m_settings.run_type != 0) {
    select_kernel = vc4;
  }

#ifdef QPU_MODE
  if (!m_settings.compile_only) {
    if (Platform::use_main_memory() && m_settings.run_type == 0) {
      Log::info << "Main memory selected in QPU mode, running on emulator instead of QPU.";
      m_settings.run_type = 1;
      select_kernel = vc4;
    }
  }
#endif

  if (m_settings.run_type != 0 || Platform::run_vc4()) {   // Compile vc4
    //Log::warn << "compile_init for vc4";
    select_kernel = vc4;
  } else {                                                 // Compile v3d
    //Log::warn << "compile_init for v3d";
    select_kernel = v3d;
  }

  assert(select_kernel != None);  

  if (select_kernel == vc4) {
    Platform::compiling_for_vc4(true);
    m_driver.reset(new vc4::KernelDriver);
  } else {
    Platform::compiling_for_vc4(false);
    m_driver.reset(new v3d::KernelDriver);
  }

  driver().init_compile();
}


bool BaseKernel::has_errors() const {
 return has_driver() && driver().has_errors();
}


std::string BaseKernel::dump() {
  return driver().dump();
}


void BaseKernel::run(bool wait_complete) {
  assert(m_driver.get() != nullptr);

#ifdef QPU_MODE
  if (Platform::use_main_memory()) {
    if (m_driver->is_v3d()) {
       if (!m_settings.compile_only) {
        fatal("Main memory selected in QPU mode and not compiled for vc4, can not run.");
      }
    } else {
       if (!m_settings.compile_only) {
        cdebug << "Main memory selected in QPU mode, running on emulator instead of QPU.";
      }
      m_settings.run_type = 1;
    }
  }
#else
  if (m_settings.run_type == 0) {
    assert(!m_driver->is_v3d());
    debug("Not compiled for QPU, running on emulator instead of QPU.");
    m_settings.run_type = 1;
  }
#endif

  m_settings.startPerfCounters();

  if (m_settings.compile_only) {
    // A kernel can be called multiple times, show warning only on first attempt
    static bool showed_msg = false;

    if (!showed_msg) {
      Log::warn << "BaseKernel::run(): Compile-only selected, not running.";
      showed_msg = true;
    }
  } else {

    switch (m_settings.run_type) {
      case 0: qpu(wait_complete); break;
      case 1: emu(); break;
      case 2: interpret(); break;
    }
  }

  m_settings.stopPerfCounters();

  m_settings.process(*this);
}


/**
 * Invoke the emulator
 *
 * The emulator runs vc4 code.
 */
void BaseKernel::emu() {
  assert(!m_settings.compile_only);    // Paranoia

  if (driver().has_errors()) {
    warning("Not running on emulator, there were errors during compile.");
    return;
  }

  assert(uniforms.size() != 0);
  emulate(numQPUs(), driver().targetCode(), driver().numVars(), uniforms, getBufferObject());
}


/**
 * Invoke the interpreter
 */
void BaseKernel::interpret() {
  assert(!m_settings.compile_only);    // Paranoia
  assertq(!driver().is_v3d(), "Can not run interpreter on v3d");

  if (driver().has_errors()) {
    warning("Not running interpreter, there were errors during compile.");
    return;
  }

  assert(uniforms.size() != 0);
  interpreter(numQPUs(), driver().sourceCode(), driver().numVars(), uniforms, getBufferObject());
}


/**
 * Invoke kernel on physical QPU hardware
 */
void BaseKernel::qpu(bool wait_complete) {
  assert(!m_settings.compile_only);    // Paranoia

#ifdef QPU_MODE
  driver().invoke(numQPUs(), uniforms, wait_complete);
#else
  fatal("qpu(): QPU mode not enabled, can not run on hardware.");
#endif  // QPU_MODE
}


std::string BaseKernel::compile_info() const {
  std::string ret;

  ret << "\n"
      << "Compile info\n"
      << "============\n";

  if (!has_driver()) {
    ret << "No kernel drivers enabled\n\n";
  } else {
    ret << driver().kernel_type_str() << ":\n";
  }

  ret << driver().compile_info() << "\n\n";

  return ret;
}


std::string BaseKernel::dump_compile_data() {
  return driver().dump_compile_data();
}


std::string BaseKernel::info() const {
  std::string ret;

  if (has_driver() ) {
    ret << "  " << driver().kernel_type_str() << " kernel: "
        << driver().kernel_size() << " instructions\n";
  } else {
    ret << "  kernel not present\n";
  }

  return ret;
}

}  // namespace V3DLib
