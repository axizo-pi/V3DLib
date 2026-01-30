#include "KernelDriver.h"
#include <iostream>                 // cout
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Source/StmtStack.h"
#include "Source/Translate.h"
#include "Source/Lang.h"            // initStmt
#include "Support/Timer.h"
#include "Target/instr/Mnemonics.h"
#include "Support/Helpers.h"        // to_file

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
		//to_file("stack.txt", stmtStack().dump());

    compile_intern();
    m_numVars = VarGen::count();
  } catch (V3DLib::Exception const &e) {
		// TODO: I have the impression that this block is not reached any more.
		//       I think that V3DLib::Exception is on its way out.

    std::string e_msg = e.msg();
		Log::warn << "Exception e_msg: " << e_msg;

    std::string msg   = "Exception occurred during compilation: ";
    msg << e_msg;

    clearStack();

    if (e_msg.compare(0, 5, "ERROR") == 0) {
      errors << msg;
    } else {
      m_compile_data = compile_data;
      throw;  // Must be a fatal()
    }
  } catch (std::runtime_error const &e) {
		// NOTE: Following handling derived from previous block for V3DLib::Exception

    std::string e_msg = e.what();
		Log::warn << "runtime_error e_msg: " << e_msg;

    std::string msg = "runtime error occurred during compilation: ";

    msg << e_msg;

    clearStack();

    errors << msg;
  } catch (...) {
		std::string msg = "Unknown exception occurred during compilation";
		Log::cerr << msg;
    errors << msg;
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
      << m_targetCode.mnemonics(true);

	return ret;
}


std::string KernelDriver::dump_compile_data() const {
  return m_compile_data.dump()
  	+ ::title("ACC usage")
  	+ m_targetCode.check_acc_usage();
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


void KernelDriver::wait_complete() {
	/* Intentionally left blank */
}

}  // namespace V3DLib
