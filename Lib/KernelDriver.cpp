#include "KernelDriver.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Source/StmtStack.h"
#include "Source/Translate.h"
#include "Source/Lang.h"            // initStmt()
#include "Target/instr/Mnemonics.h"

namespace V3DLib {

using ::operator<<;  // C++ weirdness

/**
 * NOTE: Don't clean up `body` here, it's a pointer to the top of the AST.
 */
KernelDriver::~KernelDriver() {}


/**
 * Reset the state for compilation
 *
 */
void KernelDriver::init_compile() {
  initStack(m_stmtStack);

  VarGen::reset();
  resetFreshLabelGen();

#ifdef OUTPUT_COMPILEDATA
  compile_data.clear();
#endif // OUTPUT_COMPILEDATA

  // Initialize reserved general-purpose variables.
  // Assignment to Int are required; unit tests fail otherwise.
  Int qpuId    = getUniformInt();  comment("QPU id");
  Int qpuCount = getUniformInt();  comment("Num QPUs");
  
  init_uniforms();  // v3d only

  //Log::warn << "init_compile stack:\n" << m_stmtStack.dump();
}


void KernelDriver::obtain_ast() {
  clearStack();

  if (m_stmtStack.size() != 1) {
    info << "Expected exactly one item on stmtstack; perhaps an 'End'-statement is missing." << thrw;
  }

  //Log::warn << "obtain_ast stack:\n" << m_stmtStack.dump();
  m_body = *m_stmtStack.pop();
}


/**
 * Entry point for compilation of source code to target code.
 *
 * This method is here to just handle thrown exceptions.
 */
void KernelDriver::compile(std::function<void()> create_ast) {
  try {
    create_ast();
    compile_intern();

    m_numVars = VarGen::count();
  } catch (V3DLib::Exception const &e) {
    // TODO: Looks like this one is not used, cleanup?
    std::string e_msg = e.what();
    Log::warn << "V3DLib::Exception caught: " << e_msg;

    std::string msg = "Exception occurred during compilation: ";
    msg << e_msg;

    clearStack();

    if (e_msg.compare(0, 5, "ERROR") == 0) {
      errors << msg;
    } else {
#ifdef OUTPUT_COMPILEDATA
      m_compile_data = compile_data;
#endif // OUTPUT_COMPILEDATA
      throw;  // Must be a fatal()
    }
  } catch (std::runtime_error const &e) {
    clearStack();
    errors << e.what();
  } catch (...) {
    std::string msg = "Unknown exception occurred during compilation";
    Log::cerr << msg;
    errors << msg;
  }

  handle_errors();

#ifdef OUTPUT_COMPILEDATA
  m_compile_data = compile_data;
#endif // OUTPUT_COMPILEDATA
}


/**
 * @return true if errors present, false otherwise
 */
bool KernelDriver::handle_errors() {
  if (errors.empty()) return false;
  Log::cout_timestamp ts(false);

  std::string buf;

  buf << "Errors encountered during compilation and/or encoding:\n";

  for (auto const &err : errors) {
    buf << "  * " << err << "\n";
  }

  buf << "\nNot running the kernel\n";

  cerr << buf;

  return true;      
}


/**
 * Return AST representing the source code
 *
 * But it's not really a 'tree' anymore, it's a top-level sequence of statements
 */
Stmts &KernelDriver::sourceCode() {
  return m_body;
}


/**
* @brief Output a human-readable representation of the source and target code.
*
* @param filename  if specified, print the output to this file. Otherwise, print to stdout
*/
std::string KernelDriver::dump() {
  std::string ret;

  if (has_errors()) {
    ret << "=== There were errors during compilation, the output here is likely incorrect or incomplete  ===\n"
        << "=== Encoding and displaying output as best as possible                                       ===\n"
        << "\n\n";
  }

  ret << "Opcodes for " << kernel_type_str() << "\n"
      << "===============\n"
      << emit_opcodes()
      << "\n";

  ret << "Source for " << kernel_type_str() << "\n"
      << "===============\n"
      << m_body.dump()
      << "\n"

      << "Target for " << kernel_type_str() << "\n"
      << "===============\n"
      << m_targetCode.dump();

  return ret;
}


#ifdef OUTPUT_COMPILEDATA

std::string KernelDriver::dump_compile_data() const {
  return m_compile_data.dump()
    + ::title("ACC usage")
    + m_targetCode.check_acc_usage();
}

#endif // OUTPUT_COMPILEDATA


std::string KernelDriver::compile_info() const {
  std::string ret;

  ret << "  compile num generated variables: " << numVars() << "\n"
#ifdef OUTPUT_COMPILEDATA
      << "  num accs introduced            : " << numAccs() << "\n"
#endif // OUTPUT_COMPILEDATA
      << "  num compile errors             : " << errors.size();

  return ret;
}


std::string KernelDriver::kernel_type_str() const {
  switch(kernel_type()) {
    case vc4: return "vc4";
    case vc6: return "vc6";
    case vc7: return "vc7";
    default:  assert(false); return "none";  // Should never occur
  }
}


void KernelDriver::wait_complete() {
  /* Intentionally left blank */
}

}  // namespace V3DLib
