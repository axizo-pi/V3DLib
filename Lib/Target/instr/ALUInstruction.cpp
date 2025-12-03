#include "ALUInstruction.h"
#include "Support/debug.h"

namespace V3DLib {

bool ALUInstruction::noOperands() const {
  return Oper::noOperands(op);
}


bool ALUInstruction::oneOperand() const {
  return Oper::oneOperand(op);
}


}  // namespace V3DLib
