#include "ALUInstruction.h"
#include "Source/OpItems.h"
#include "Support/debug.h"

namespace V3DLib {

int ALUInstruction::num_operands() const {
	int num = OpItems::num_operands_by_op(op.value());
	assert(num != -1);

	if (num <= 1) {
		assert(srcB.is_none());
	}

	if (num == 0) {
		assert(srcA.is_none());
	}

	return num;
}

}  // namespace V3DLib
