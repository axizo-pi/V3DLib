#include "ALUInstruction.h"
#include "Support/debug.h"

namespace V3DLib {

bool ALUInstruction::noOperands() const {
  if (srcA.is_none() && srcB.is_none())  {
	  assert(                           // Pedantry: these should be the only operations with no operands
			op.value() == ALUOp::A_TMUWT ||
			op.value() == ALUOp::A_TIDX  ||
			op.value() == ALUOp::A_EIDX
		);

	  return true;
	} else {
		return false;
	}
}


bool ALUInstruction::oneOperand() const {
  if (srcA.is_none() && srcB.is_none()) return false;

  if (srcB.is_none()) {
	  assert(                            // Pedantry: these should be the only operations with one operand
			op.value() == ALUOp::A_FSIN   ||
			op.value() == ALUOp::A_FFLOOR ||
			op.value() == ALUOp::A_MOV       // vc7 
		);

  	return true;
	} else {
		return false;
	}
}


}  // namespace V3DLib
