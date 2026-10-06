#include "Compile.h"
#include "CodeStruct.h"
#include "Source/Lang.h"  // comment()
#ifdef OUTPUT_COMPILEDATA
#include "Common/CompileData.h"
#endif // OUTPUT_COMPILEDATA

namespace V3DLib {

using ::operator<<;  // C++ weirdness

Compile::Compile() {
  m_code_struct = new CodeStruct;
#ifdef OUTPUT_COMPILEDATA
  m_compile_data = new CompileData;
#endif // OUTPUT_COMPILEDATA
}

/**
 * @brief class dtor
 *
 * Thankfully, this is also called after derived virtual dtor's.
 */
Compile::~Compile() {
  //warn << "Called Compile base dtor";
  delete m_code_struct;
#ifdef OUTPUT_COMPILEDATA
  delete m_compile_data;
#endif // OUTPUT_COMPILEDATA
}

std::string Compile::kernel_type_str() const {
	return Platform::vc_type_str(kernel_type());
}


CodeStruct &Compile::code_struct() {
  assert(m_code_struct != nullptr);
  return *m_code_struct;
}


CodeStruct const &Compile::code_struct() const {
  assert(m_code_struct != nullptr);
  return *m_code_struct;
}


/**
 * Entry point for compilation of source code to target code.
 *
 * This method is here to just handle thrown exceptions.
 */
void Compile::compile(std::function<void()> create_ast) {
  try {
		warn << "Compile::compile() compiling for: " << kernel_type_str();

    create_ast();
    compile_intern();

    m_numVars = VarGen::count();
  } catch (V3DLib::Exception const &e) {
    // Catches (at least) "FATAL: Failed to allocate vc4 shared memory."

    std::string e_msg = e.what();
    Log::warn << "V3DLib::Exception caught: " << e_msg;

    std::string msg = "Exception occurred during compilation: ";
    msg << e_msg;

    clearStack();

    if (e_msg.compare(0, 5, "ERROR") == 0) {
      m_errors << msg;
    } else {
#ifdef OUTPUT_COMPILEDATA
      delete m_compile_data;
      m_compile_data = new CompileData(compile_data);
#endif // OUTPUT_COMPILEDATA
      throw;  // Must be a fatal()
    }
  } catch (std::runtime_error const &e) {
    clearStack();
    m_errors << e.what();
  } catch (...) {
    std::string msg = "Unknown exception occurred during compilation";
    Log::cerr << msg;
    m_errors << msg;
  }

  handle_errors();

#ifdef OUTPUT_COMPILEDATA
  delete m_compile_data;
  m_compile_data = new CompileData(compile_data);
#endif // OUTPUT_COMPILEDATA
}


/**
 * Reset the state for compilation
 *
 */
void Compile::init_compile() {
  auto &cs = code_struct();

  cs.init();
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
}


std::string Compile::compile_info() const {
  std::string ret;

  ret << "  compile num generated variables: " << numVars() << "\n"
#ifdef OUTPUT_COMPILEDATA
      << "  num accs introduced            : " << numAccs() << "\n"
#endif // OUTPUT_COMPILEDATA
      << "  num compile errors             : " << m_errors.size();

  return ret;
}


/**
 * @return true if errors present, false otherwise
 */
bool Compile::handle_errors() {
  if (m_errors.empty()) return false;
  Log::cout_timestamp ts(false);

  std::string buf;
  buf << "\n\nErrors encountered during compilation and/or encoding:\n";

  for (auto const &err : m_errors) {
    buf << "  * " << err << "\n";
  }

  buf << "Not running the kernel\n";

  cerr << buf;

  return true;      
}


/**
* @brief Output a human-readable representation of the source and target code.
*
* @param filename  if specified, print the output to this file. Otherwise, print to stdout
*/
std::string Compile::dump() {
  auto &cs = code_struct();
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
      << cs.m_body.dump()
      << "\n"

      << "Target for " << kernel_type_str() << "\n"
      << "===============\n"
      << cs.m_targetCode.dump();

  return ret;
}


#ifdef OUTPUT_COMPILEDATA

std::string Compile::dump_compile_data() const {
  std::string ret;
  assert(m_compile_data != nullptr);
  ret = m_compile_data->dump();

  // vc7 has no accumulators, don't display
  if (!Platform::compile::for_vc7()) {
    ret << ::title("ACC usage")
        << " - This is for final Target source.\n"
        << " - Index is line number, digits are accumulator indexes.\n";

    if (Platform::compile::for_vc6()) {
      ret  << " - vc6: The load immediate instruction can potentially also use acc 0 and 1.\n"
           << "   Logic requires that these acc's are always flagged.\n";
    }

    ret << "\n"
        << code_struct().m_targetCode.dump_acc_usage();
  }

  return ret;
}


int Compile::numAccs() const {
  assert(m_compile_data != nullptr);
  return m_compile_data->num_accs_introduced;
}

#endif // OUTPUT_COMPILEDATA

}  // namespace V3DLib
