#include "BaseKernel.h"
#include "vc4/Compile.h"
#include "v3d/Compile.h"
#include "Emulator/Interpreter.h"  // interpreter()
#include "Emulator/Emulator.h"     // emulate()

using VCType = V3DLib::Platform::VCType;

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


V3DLib::Compile const &BaseKernel::compile() const {
  assert(m_compile != nullptr);
  return *m_compile;
}


V3DLib::Compile &BaseKernel::compile() {
  assert(m_compile != nullptr);
  return *m_compile;
}


void BaseKernel::compile_init() {
  assert(m_compile.get() == nullptr);

  VCType select_kernel = VCType::UNKNOWN;

#ifdef V3D_ALLOW_INTERPRET
  if (Platform::run_vc4()) {
    select_kernel = VCType::vc4;
  } else {
    select_kernel = v3d;
  }
#else
  if (m_settings.run_type != QPU) {
    select_kernel = VCType::vc4;
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
      select_kernel = VCType::vc4;
    }
  }

  if (m_settings.run_type != QPU) {
    select_kernel = VCType::vc4;
  } else {
    select_kernel = Platform::vc_type();
  }
#endif

  assert(select_kernel != VCType::UNKNOWN);  

  if (select_kernel == VCType::vc4) {
    Platform::compile::start(select_kernel);
    m_compile.reset(new vc4::Compile);
  } else {
    Platform::compile::start(select_kernel);
    m_compile.reset(new v3d::Compile);
  }
}


bool BaseKernel::has_errors() const {
 return compile().has_errors();
}


std::string BaseKernel::dump() {
  return compile().dump();
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
#ifdef V3D_ALLOW_INTERPRET
    if (m_settings.run_type == QPU) {
      warn << "Main memory selected in QPU mode, running on emulator instead of QPU.";
      m_settings.run_type = Emulator;
    }
#else
    if (compile().is_v3d()) {
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
#endif
  }

  bool do_execute = true;

  if (m_settings.compile_only) {
    // A kernel can be called multiple times, show warning only on first attempt
    static bool showed_msg = false;

    if (!showed_msg) {
      warn << "BaseKernel::run(): Compile-only selected, not running.";
      showed_msg = true;
    }

    do_execute = false;
  }

  if (do_execute) {
    m_settings.startPerfCounters();

    switch (m_settings.run_type) {
      case 0: qpu(wait_complete); break;
      case 1: interpret();        break;
      case 2: emu();              break;
      case 3: emu(true);          break;
    }

    m_settings.stopPerfCounters();
  }

  m_settings.dump_code(*this);
}


/**
 * Invoke the emulator
 *
 * The emulator runs vc4 code.
 */
void BaseKernel::emu(bool do_debug) {
  if (m_settings.compile_only) return;

  if (compile().has_errors()) {
    warn << "Not running on emulator, there were errors during compile.";
    return;
  }

  assertq(compile().kernel_type() == VCType::vc4, "Can not run interpreter for v3d");
  assert(uniforms.size() != 0);

  Platform::run_emulator(compile().kernel_type());

  emulate(
    numQPUs(),
    compile().code_struct(),
    compile().numVars(),
    uniforms,
    getBufferObject(),
    do_debug
  );

  Platform::done_emulating();
}


/**
 * Invoke the interpreter
 */
void BaseKernel::interpret() {
  assert(!uniforms.empty());
  assert(!m_settings.compile_only);    // Paranoia

  if (compile().has_errors()) {
    cerr << "Not running interpreter, there were errors during compile.";
    return;
  }

#ifdef V3D_ALLOW_INTERPRET
  warn << "interpret allowing v3d";

  warn << "interpret() "
       << "is_v3d: "  << compile().is_v3d()  << ", "
       << "run vc4: " << Platform::run_vc4();
#else
  assertq(compile().kernel_type() == VCType::vc4, "Can not run interpreter for v3d");
#endif


  Platform::run_emulator(compile().kernel_type());

  interpreter(
    numQPUs(),
    compile().code_struct(),
    compile().numVars(),
    uniforms,
    getBufferObject()
  );

  Platform::done_emulating();
}


/**
 * Invoke kernel on physical QPU hardware
 */
void BaseKernel::qpu(bool wait_complete) {
  bool do_execute = true;
  std::string err;

  if (m_settings.compile_only) {
    err << "BaseKernel::qpu(): Compile-only selected, not running.";
    do_execute = false;
  } else
  if (compile().is_v3d() && Platform::vc_type() == Platform::vc4) {
    err << "BaseKernel::qpu(): Trying to run v3d code on vc4 platform, not running.";
    do_execute = false;
  } else
  if (!compile().is_v3d() && Platform::vc_type() != Platform::vc4) {
    err << "BaseKernel::qpu(): Trying to run vc4 code on v3d platform, not running.";
    do_execute = false;
  }

  if (!do_execute) assert(!err.empty());
  assertq(do_execute, err);

  s_qpu_call_count++;
  compile().invoke(numQPUs(), uniforms, wait_complete);
}


void BaseKernel::wait_complete() { compile().wait_complete(); }


int BaseKernel::qpu_call_count() {
  return s_qpu_call_count;
}


std::string BaseKernel::compile_info() const {
  std::string ret;

  ret << "\n"
      << "Compile info\n"
      << "============\n";

  if (!has_compile()) {
    ret << "No compile member enabled\n\n";
  } else {
    ret << compile().kernel_type_str() << ":\n";
  }

  ret << compile().compile_info() << "\n\n";

  return ret;
}


#ifdef OUTPUT_COMPILEDATA
std::string BaseKernel::dump_compile_data() {
  return compile().dump_compile_data();
}
#endif // OUTPUT_COMPILEDATA


std::string BaseKernel::info() const {
  std::string ret;

  if (has_compile() ) {
    ret << "  " << compile().kernel_type_str() << " kernel: "
        << compile().kernel_size() << " instructions\n";
  } else {
    ret << "  compile member not present\n";
  }

  return ret;
}

}  // namespace V3DLib
