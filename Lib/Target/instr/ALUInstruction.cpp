#include "ALUInstruction.h"
#include "Source/OpItems.h"
#include "Support/debug.h"

namespace V3DLib {

int ALUInstruction::num_operands() const {
  int num = OpItems::num_operands_by_op(op.value());
  assert(num != -1);

  // The operand values are completely undefined for <= operand, ignor
  return num;
}

}  // namespace V3DLib
