#include "KernelDriver.h"
#include <iostream>            // cout
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Source/StmtStack.h"
#include "Source/Pretty.h"
#include "Source/Translate.h"
#include "Source/Lang.h"       // initStmt
#include "Support/Timer.h"
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
  Pointer::reset_increment();
  compile_data.clear();

  // Initialize reserved general-purpose variables
  Int qpuId, qpuCount;
  qpuId    = getUniformInt();  comment("QPU id");
  qpuCount = getUniformInt();  comment("Num QPUs");

  if (!Platform::compiling_for_vc4()) {
    Int devnull = getUniformInt();  comment("devnull");
  }
}


void KernelDriver::obtain_ast() {
  clearStack();

  if (m_stmtStack.size() != 1) {
    std::string buf = "Expected exactly one item on stmtstack; perhaps an 'End'-statement is missing.";
    error(buf, true);
  }

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
    std::string msg = "Exception occured during compilation: ";
    msg << e.msg();

    clearStack();

    if (e.msg().compare(0, 5, "ERROR") == 0) {
      errors << msg;
    } else {
      m_compile_data = compile_data;
      throw;  // Must be a fatal()
    }

  }

  m_compile_data = compile_data;
}


std::string KernelDriver::get_errors() const {
  std::string ret;

  for (auto const &err : errors) {
    ret << "  " << err << "\n";
  }

  return ret;
}


/**
 * @return true if errors present, false otherwise
 */
bool KernelDriver::handle_errors() {
  using std::cout;
  using std::endl;

  if (errors.empty()) return false;

  cout << "Errors encountered during compilation and/or encoding:\n"
       << get_errors()
       << "\nNot running the kernel" << endl;

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
std::string KernelDriver::dump() const {
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
      << m_targetCode.mnemonics(true);

	return ret;
}


std::string KernelDriver::dump_compile_data() const {
  return m_compile_data.dump()
  	+ ::title("ACC usage")
  	+ m_targetCode.check_acc_usage();
}


void KernelDriver::invoke(int numQPUs, IntList &params) {
  assert(params.size() != 0);

  if (handle_errors()) {
    fatal("Errors during kernel compilation/encoding, can't continue.");
  }

   // Invoke kernel on QPUs
  invoke_intern(numQPUs, params);
}


std::string KernelDriver::compile_info() const {
  std::string ret;

  ret << "  compile num generated variables: " << numVars() << "\n"
      << "  num accs introduced            : " << numAccs() << "\n"
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

}  // namespace V3DLib
