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

class UniformConstants {
public:
	UniformConstants();
	void reset();
	void last_uniform(Stmts &source);
	void add_uniforms(Stmts &source);
	void load(Data &unif, int &offset);
	int size() const { return (int) m_list.size(); }
	int get(float in_val);

private:	

	struct UniformConstant {
		UniformConstant(float in_val, std::string const &in_comment = "");

		uint32_t    val;
		std::string comment;
		int         var_id = -1;
	};

	int m_last_uniform = -1;
	std::vector<UniformConstant> m_list;
};

extern UniformConstants uniform_constants;

}  // namespace v3d
}  // namespace V3DLib

#endif  // QPU_MODE
#endif // _LIB_V3D_UNIFORMCONSTANTS_H
