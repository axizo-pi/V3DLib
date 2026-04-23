#ifndef _LIB_KERNELDRIVER_H
#define _LIB_KERNELDRIVER_H
#include <vector>
#include <string>
#include <functional>
#include "Common/CompileData.h"
#include "Source/StmtStack.h"
#include "Invoke.h"

namespace V3DLib {

class KernelDriver {
public:
  enum KernelType {
    vc4,
    vc6,
    vc7
  };

  KernelDriver() = default;
  KernelDriver(KernelDriver &&k) = default;
  virtual ~KernelDriver();

  void init_compile();
  void compile(std::function<void()> create_ast);
  virtual void encode() = 0;
  virtual void invoke(int numQPUs, IntList &params, bool wait_complete = true) = 0;
  bool has_errors() const { return !errors.empty(); }
  int numVars() const { return m_numVars; }
  Instr::List &targetCode() { return m_targetCode; }
  Stmts &sourceCode();

  Code const &code() const { return m_code; }

  std::string dump();
  std::string compile_info() const;

  bool        is_v3d()      const { return m_type == vc6 || m_type == vc7; }
  KernelType  kernel_type() const { return m_type; }
  std::string kernel_type_str() const;
  virtual int kernel_size() const = 0; 
  virtual void wait_complete();

protected:
  KernelType  m_type;
  Instr::List m_targetCode; // Target code generated from AST
  Stmts       m_body;       // Source code statements

  Code m_code;              // Memory region for QPU code
                            // Doesn't survive std::move, dtor gets called despite move ctor present

  std::vector<std::string> errors;

  virtual std::string emit_opcodes() { return ""; } 
  void obtain_ast();

private:
  StmtStack   m_stmtStack;
  int         m_numVars = 0;           // The number of variables in the source code for vc4

  virtual void compile_intern() = 0;

  bool handle_errors();
	virtual void init_uniforms() {}

#ifdef OUTPUT_COMPILEDATA
public:  
  std::string dump_compile_data() const;

private  
  CompileData m_compile_data;

  int numAccs() const { return m_compile_data.num_accs_introduced; }
#endif // OUTPUT_COMPILEDATA
};


}  // namespace V3DLib

#endif  // _LIB_vc4_KERNELDRIVER_H
