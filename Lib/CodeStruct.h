#ifndef _V3DLIB_CODESTRUCT_H
#define _V3DLIB_CODESTRUCT_H
#include "Source/StmtStack.h"

namespace V3DLib {

/**
 * @brief Hidden struct within Compile for holding all code on all levels.
 *
 * The goal here is to reduce file dependencies.
 */
struct CodeStruct {
  Stmts       m_body;         // Source code statements
  Instr::List m_targetCode;   // Target code generated from AST
  Code m_code;                // Memory region for QPU code

	void init();
	void obtain_ast();

	Code const &code() const { return m_code; }
	Stmts const &sourceCode() const;
  Instr::List const &targetCode() const { return m_targetCode; }

private:
  StmtStack   m_stmtStack;
};

}  // namespace V3DLib

#endif // _V3DLIB_CODESTRUCT_H
