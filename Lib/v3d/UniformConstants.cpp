#ifdef QPU_MODE

#include "UniformConstants.h"
#include "Support/basics.h"
#include "Source/Int.h"
#include "Source/Lang.h"            // comment()
#include "Source/StmtStack.h"
#include "Target/instr/Mnemonics.h"

namespace V3DLib {
namespace v3d {

UniformConstants::UniformConstants() {}


void UniformConstants::reset() {
	m_last_uniform = -1;
	m_list.clear();
}


/**
 * @brief Retain the index of the last uniform
 *
 * This is to add the uniform constants at the proper location later on.
 */
void UniformConstants::last_uniform(Stmts &source) {
	m_last_uniform = source.lastUniform(false);
}


/**
 * @brief Insert collected constants into the uniforms list of
 *        the source language program.
 *
 * This assumes that the var index generation has **not** been reset.
 */
void UniformConstants::add_uniforms(Stmts &source) {
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
 * @brief Load constant into the uniforms Data array,
 *        to pass into the kernel.
 */
void UniformConstants::load(Data &unif, int &offset) {
	if (m_list.empty()) return;
	//warn << "Called UniformConstants::load()";

	for (int i = 0; i < (int) m_list.size(); ++i) {
  	//warn <<  "val: " << m_list[i].val; 
  	unif[offset++] = m_list[i].val; 
	}
}


/**
 * Return var index for given value
 *
 * This assumes two passes; the first pass returns -1.
 * the second pass returns the proper var index.
 */
int UniformConstants::get(float val) {
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


UniformConstants::UniformConstant::UniformConstant(float in_val, std::string const &in_comment) :
	comment(in_comment)
{
	// Store float into uint32_t
	val = *((uint32_t *) &in_val);
}


UniformConstants uniform_constants;

}  // namespace v3d
}  // namespace V3DLib

#endif  // QPU_MODE
