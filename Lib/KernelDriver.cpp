#include "KernelDriver.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Source/StmtStack.h"
#include "Source/Translate.h"
#include "Source/Lang.h"            // initStmt()
#include "Support/Timer.h"
#include "Target/instr/Mnemonics.h"
#include "Support/Helpers.h"        // to_file(), uniforms_reversed()

namespace V3DLib {

using ::operator<<;  // C++ weirdness

namespace {

/**
 * @brief Reverse the kernel parameter indexes if necessary.
 *
 * On Debian Trixie, gnu c++ v14.2.0, the initialization order of kernel parameters
 * is **reversed**. Thus, the last parameter in the kernel function call is initialized first.
 *
 * This screws up initialization of the uniforms in the kernel.
 * The goal of this function is to reverse the parameter indexes, so that the kernel functions
 * again as expected.
 */
MAYBE_UNUSED void reverse_uniforms(StmtStack &stack) {
  if (!uniforms_reversed()) return;
  warn << "Doing reverse_uniforms()";

  Stmts uniforms;

  //
  // Retrieve the uniforms loading.
  //
  // Also checks that uniforms are be only in the start of the kernel code.
  //
  stack.each([&uniforms] (Stmts const &items) {
    bool check_start_uniforms = true;

    for (int i = 0; i < (int) items.size(); i++) {
      auto &item = *items[i];
      std::string tmp = item.dump();

      if (check_start_uniforms) {
        if (contains(tmp, "Uniform")) {
          //warn << "uniform! lhs: " << id;
          uniforms.push_back(items[i]);
        } else {
          check_start_uniforms = false;
        }
      } else {
        // Check the rest of the code for uniforms, not expecting any
        assert(!contains(tmp, "Uniform"));
      }
    }
  });

  warn << "uniforms:\n" << uniforms.dump();

  // Check first two parameters
  assert(contains(uniforms[0]->dump(true) , "QPU id"));
  assert(contains(uniforms[1]->dump(true) , "Num QPUs"));

  // Reverse the rest of the uniforms, which are the kernel parameters
  for (int i = 2; i < (int) uniforms.size(); ++i) {
    int j = (int) (uniforms.size() - 1) - (i - 2);
    if (j <= i) break;
    warn << "i,j: " << i << ", " << j;

    // Switch the values of the i and j items
    auto tmp_lhs = uniforms[i]->lhs();
    auto tmp_rhs = uniforms[i]->rhs();

    uniforms[i]->lhs(uniforms[j]->lhs());
    uniforms[i]->rhs(uniforms[j]->rhs());

    uniforms[j]->lhs(tmp_lhs);
    uniforms[j]->rhs(tmp_rhs);
  }

  warn << "uniforms post:\n" << uniforms.dump();
}

} // anon namespace

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
  compile_data.clear();

  // Initialize reserved general-purpose variables
  Int qpuId, qpuCount;
  qpuId    = getUniformInt();  comment("QPU id");
  qpuCount = getUniformInt();  comment("Num QPUs");

  if (!Platform::compiling_for_vc4()) {
    Int devnull = getUniformInt();  comment("devnull");
  }

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
    //reverse_uniforms(m_stmtStack);

    compile_intern();
    m_numVars = VarGen::count();
  } catch (V3DLib::Exception const &e) {
    // TODO: Choose between this exception type and runtime_error
    //       Looks like this one is not used.
    std::string e_msg = e.what();
    Log::warn << "V3DLib::Exception caught: " << e_msg;

    std::string msg = "Exception occurred during compilation: ";
    msg << e_msg;

    clearStack();

    if (e_msg.compare(0, 5, "ERROR") == 0) {
      errors << msg;
    } else {
      m_compile_data = compile_data;
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

  m_compile_data = compile_data;
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
