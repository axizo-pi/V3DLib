#ifndef _LIB_V3D_UNIFORMCONSTANTS_H
#define _LIB_V3D_UNIFORMCONSTANTS_H
#ifdef QPU_MODE

#include "Common/SharedArray.h"    // Data
#include "Source/Stmt.h"
#include "Target/instr/Instr.h"
#include <vector>
#include <string>

namespace V3DLib {
namespace v3d {

struct UniformConstant {
	UniformConstant(float in_val, std::string const &in_comment = "");

	uint32_t    val;
	std::string comment;
	int         var_id = -1;
};


class UniformConstants : public std::vector<UniformConstant> {
public:	
	void load(Data &unif, int &offset) const;
};


class UniformConstantsHandler {
public:
	UniformConstantsHandler();
	void reset();
	void last_uniform(Stmts &source);
	void add_uniforms(Stmts &source);
	bool empty() const { return m_list.empty(); }
	int get(float in_val);
	UniformConstants list() const { return m_list; }

private:	
	int m_last_uniform = -1;
	UniformConstants m_list;
};

// Needs to be global during encoding; see RegOrImm
extern UniformConstantsHandler uniform_constants;

}  // namespace v3d
}  // namespace V3DLib

#endif  // QPU_MODE
#endif // _LIB_V3D_UNIFORMCONSTANTS_H
