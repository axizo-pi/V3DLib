#include "BaseKernel.h"
#include "vc4/Compile.h"
#include "v3d/Compile.h"
#include "Support/basics.h"
#include "Emulator/Interpreter.h"  // interpreter()
#include "Emulator/Emulator.h"     // emulate()

/**
 * /file
 * Basic Kernel class.
 */

namespace V3DLib {

using ::operator<<;  // C++ weirdness

namespace {

int s_qpu_call_count =0;

}  // anon namespace

BaseKernel::BaseKernel(BaseSettings const &settings) : m_settings(settings) {}


bool BaseKernel::has_compile() const { return m_compile.get() != nullptr; }


V3DLib::Compile &BaseKernel::compiler() const {
  assert(m_compile != nullptr);
  return *m_compile;
}


V3DLib::Compile &BaseKernel::compiler() {
  assert(m_compile != nullptr);
  return *m_compile;
}


void BaseKernel::compile_init() {
  //warn << "Called compile_init()";
  assert(m_compile.get() == nullptr);

  enum SelectKernel {
    None,
    vc4,
    v3d
  };

  SelectKernel select_kernel = None;

  if (m_settings.run_type != QPU) {
    select_kernel = vc4;
  }

  if (!m_settings.compile_only) {
    if (Platform::use_main_memory() && m_settings.run_type == QPU) {
      static int warn_count = 0;

      if (warn_count == 0) {
        warn << "Main memory selected in QPU mode, running on emulator instead of QPU. "
             << "This also applies to subsequent calls.";
        warn_count++;
      }

      m_settings.run_type = Emulator;
      select_kernel = vc4;
    }
  }

  if (m_settings.run_type != QPU || Platform::run_vc4()) {   // Compile vc4
    select_kernel = vc4;
  } else {                                                   // Compile v3d
    select_kernel = v3d;
  }

  assert(select_kernel != None);  

  if (select_kernel == vc4) {
    //warn << "BaseKernel compiling for vc4";
    Platform::compiling_for_vc4(true);
    m_compile.reset(new vc4::Compile);
  } else {
    Platform::compiling_for_vc4(false);
    m_compile.reset(new v3d::Compile);
  }
}


bool BaseKernel::has_errors() const {
 return compiler().has_errors();
}


std::string BaseKernel::dump() {
  return compiler().dump();
}


BaseKernel &BaseKernel::setMaxQPUs() {
  m_settings.setMaxQPUs();
  return *this;
}


/**
 * ==================================================
 * Notes
 * -----
 *
 * - Profiling: For v3d, practically all time goes into the
 *   underlying call `submit_csd(). The overhead of the encompassing
 *   code is about 1%.
 */
void BaseKernel::run(bool wait_complete) {
  assert(m_compile.get() != nullptr);

  if (Platform::use_main_memory()) {
    if (compiler().is_v3d()) {
       if (!m_settings.compile_only) {
        fatal("Main memory selected in QPU mode and not compiled for vc4, can not run.");
      }
    } else {
      // This is also tested in `compile_init()`. However this version is called
      // outside of unit tests.
      if (!m_settings.compile_only && (m_settings.run_type == QPU)) {
        warn << "Main memory selected in QPU mode, running on emulator instead of QPU.";
        m_settings.run_type = Emulator;
      }
    }
  }

  m_settings.startPerfCounters();

  if (m_settings.compile_only) {
    // A kernel can be called multiple times, show warning only on first attempt
    static bool showed_msg = false;

    if (!showed_msg) {
      warn << "BaseKernel::run(): Compile-only selected, not running.";
      showed_msg = true;
    }
  } else {
    switch (m_settings.run_type) {
      case 0: qpu(wait_complete); break;
      case 1: interpret();        break;
      case 2: emu();              break;
      case 3: emu(true);          break;
    }
  }

  m_settings.stopPerfCounters();
  m_settings.dump_code(*this);
}


/**
 * Invoke the emulator
 *
 * The emulator runs vc4 code.
 */
void BaseKernel::emu(bool do_debug) {
  if (m_settings.compile_only) return;

  if (compiler().has_errors()) {
    warn << "Not running on emulator, there were errors during compile.";
    return;
  }

  assert(uniforms.size() != 0);

  emulate(
    numQPUs(),
    compiler().targetCode(),
    compiler().numVars(),
    uniforms,
    getBufferObject(),
    do_debug
  );
}


/**
 * Invoke the interpreter
 */
void BaseKernel::interpret() {
  assert(!m_settings.compile_only);    // Paranoia
  assertq(!compiler().is_v3d(), "Can not run interpreter on v3d");

  if (compiler().has_errors()) {
    warn << "Not running interpreter, there were errors during compile.";
    return;
  }

  assert(uniforms.size() != 0);
  interpreter(numQPUs(), compiler().sourceCode(), compiler().numVars(), uniforms, getBufferObject());
}


/**
 * Invoke kernel on physical QPU hardware
 */
void BaseKernel::qpu(bool wait_complete) {
  assert(!m_settings.compile_only);    // Paranoia

  s_qpu_call_count++;
  compiler().invoke(numQPUs(), uniforms, wait_complete);
}


int BaseKernel::qpu_call_count() {
  return s_qpu_call_count;
}


std::string BaseKernel::compile_info() const {
  std::string ret;

  ret << "\n"
      << "Compile info\n"
      << "============\n";

  if (!has_compile()) {
    ret << "No compiler enabled\n\n";
  } else {
    ret << compiler().kernel_type_str() << ":\n";
  }

  ret << compiler().compile_info() << "\n\n";

  return ret;
}


#ifdef OUTPUT_COMPILEDATA
std::string BaseKernel::dump_compile_data() {
  return compiler().dump_compile_data();
}
#endif // OUTPUT_COMPILEDATA


std::string BaseKernel::info() const {
  std::string ret;

  if (has_compile() ) {
    ret << "  " << compiler().kernel_type_str() << " kernel: "
        << compiler().kernel_size() << " instructions\n";
  } else {
    ret << "  compiler not present\n";
  }

  return ret;
}

}  // namespace V3DLib
