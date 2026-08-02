#ifndef _V3DLIB_COMPILE_H
#define _V3DLIB_COMPILE_H
#include "Source/StmtStack.h"
#include "Source/Stmt.h"
#include "Common/CompileData.h"

namespace V3DLib {

/**
 * @brief Creation and storage of `VideoCore` code on all levels.
 */
class Compile {
public:
  enum KernelType {
    vc4,
    vc6,
    vc7
  };

  bool        is_v3d()      const { return m_type == vc6 || m_type == vc7; }
  KernelType  kernel_type() const { return m_type; }
  std::string kernel_type_str() const;

  virtual int kernel_size() const = 0; 
  virtual void encode() = 0;
  virtual void allocate() {}           // v3d

  void compile(std::function<void()> create_ast);

  Code const &code() const { return m_code; }
  Stmts &sourceCode();
  Instr::List &targetCode() { return m_targetCode; }
  int numVars() const { return m_numVars; }

  std::string dump();
  bool has_errors() const { return !m_errors.empty(); }
  std::string compile_info() const;

protected:
  KernelType  m_type;

  Stmts       m_body;         // Source code statements
  Instr::List m_targetCode;   // Target code generated from AST
  Code m_code;                // Memory region for QPU code
                              // Doesn't survive std::move, dtor gets called despite move ctor present

  void obtain_ast();
  void init_compile();
  std::vector<std::string> &errors() { return m_errors; }  // TODO remove when done
  bool handle_errors();

private:
  StmtStack   m_stmtStack;

  int         m_numVars = 0;  // The number of variables in the source code for vc4

  std::vector<std::string> m_errors;

  virtual void compile_intern() = 0;
  virtual void init_uniforms() {}
  virtual std::string emit_opcodes() { return ""; } 

#ifdef OUTPUT_COMPILEDATA
public:  
  std::string dump_compile_data() const;

private:
  CompileData m_compile_data;

  int numAccs() const { return m_compile_data.num_accs_introduced; }
#endif // OUTPUT_COMPILEDATA
};

}  // namespace V3DLib

#endif // _V3DLIB_COMPILE_H
