#ifdef QPU_MODE

#include "UniformConstants.h"
#include "Support/basics.h"
#include "Source/Int.h"
#include "Source/Lang.h"            // comment()
#include "Source/StmtStack.h"
#include "Target/instr/Mnemonics.h"

namespace V3DLib {
namespace v3d {

UniformConstantsHandler::UniformConstantsHandler() {}


void UniformConstantsHandler::reset() {
  m_last_uniform = -1;
  m_list.clear();
}


/**
 * @brief Retain the index of the last uniform
 *
 * This is to add the uniform constants at the proper location later on.
 */
void UniformConstantsHandler::last_uniform(Stmts &source) {
  m_last_uniform = source.lastUniform(false);
}


/**
 * @brief Insert collected constants into the uniforms list of
 *        the source language program.
 *
 * This assumes that the var index generation has **not** been reset.
 */
void UniformConstantsHandler::add_uniforms(Stmts &source) {
  //m_last_uniform = source.lastUniform();
  assert(m_last_uniform != -1);
  
  auto list = tempStmt([&] () {
    for (int i = 0; i < (int) m_list.size(); ++i) {
      Int tmp = getUniformInt();  comment(m_list[i].comment);
    }
  });

  int index = m_last_uniform;
  //warn << "add_uniforms: " << index;
  //warn << list.dump();

  // Retrieve the var indexes from the created code
  for (int i = 0; i < (int) list.size(); ++i) {
    int id = list[i]->lhs()->var().id();
    //warn << "lhs id: " << id;
    m_list[i].var_id = id;
  }

  source.insert(source.begin() + index + 1, list.begin(), list.end());

  //warn << "==== Post ===";
  //source.lastUniform();
}


/**
 * Return var index for given value
 *
 * This assumes two passes; the first pass returns -1.
 * the second pass returns the proper var index.
 */
int UniformConstantsHandler::get(float val) {
  assert(m_last_uniform != -1);
  uint32_t tmp = *((uint32_t *) &val);

  int index = -1;

  for (int i = 0; i < (int) m_list.size(); ++i) {
    if (tmp == m_list[i].val) {
      index = i;
      break;
    }
  }

  // If not present, add it
  if (index == -1) {
    std::string comment = std::to_string(val);
    m_list.push_back(UniformConstant(val, comment));

    index = (int) m_list.size() - 1;
  }

  // var_id is -1 for first encoding run.
  return m_list[index].var_id;
}


UniformConstant::UniformConstant(float in_val, std::string const &in_comment) :
  comment(in_comment)
{
  // Store float into uint32_t
  val = *((uint32_t *) &in_val);
}


/**
 * @brief Load constant into the uniforms Data array,
 *        to pass into the kernel.
 */
void UniformConstants::load(Data &unif, int &offset) const {
  if (empty()) return;
  //warn << "Called UniformConstants::load()";

  for (int i = 0; i < (int) size(); ++i) {
    //warn <<  "val: " << at(i).val; 
    unif[offset++] = at(i).val; 
  }
}


UniformConstantsHandler uniform_constants;

}  // namespace v3d
}  // namespace V3DLib

#endif  // QPU_MODE
