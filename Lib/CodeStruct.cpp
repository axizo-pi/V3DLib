#include "CodeStruct.h"

namespace V3DLib {

/**
 * @brief Return AST representing the source code
 *
 * But it's not really a 'tree' anymore, it's a top-level sequence of statements
 */
Stmts const &CodeStruct::sourceCode() const { return m_body; }


void CodeStruct::init() {
  initStack(m_stmtStack);
}


void CodeStruct::obtain_ast() {
  clearStack();

  if (m_stmtStack.size() != 1) {
    info << "Expected exactly one item on stmtstack; perhaps an 'End'-statement is missing." << thrw;
  }

  m_body = *m_stmtStack.pop();
}

}  // namespace V3DLib
