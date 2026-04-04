#include "OpItems.h"
#include <vector>
#include "Support/basics.h"

namespace V3DLib {

using namespace Target;

namespace v3d {
namespace instr {

bool OpItems::uses_add_alu(Instr const &instr) {
  if (instr.tag != ALU) return false;
  auto op = instr.ALU.op.value();
  op_item const *item = op_items_find_by_op(op);
  assert(item != nullptr);

  if (!item->has_add_op) return false;
  return true;
}


bool OpItems::uses_mul_alu(Instr const &instr) {
  if (instr.tag != ALU) return false;
  auto op = instr.ALU.op.value();
  op_item const *item = op_items_find_by_op(op);
  assert(item != nullptr);

  if (!item->has_mul_op) return false;
  return true;
}


bool OpItems::get_add_op(ALUInstruction const &add_alu, v3d_qpu_add_op &dst, bool strict) {
  auto op = add_alu.op.value();
  op_item const *item = op_items_find_by_op(op, strict);
  assert(item != nullptr);

  if (!item->has_add_op) return false;
  dst = item->add_op;
  return true;
}


bool OpItems::get_mul_op(ALUInstruction const &add_alu, v3d_qpu_mul_op &dst ) {
  auto op = add_alu.op.value();
  op_item const *item = op_items_find_by_op(op);
  assert(item != nullptr);

  if (!item->has_mul_op) return false;
  dst = item->mul_op;
  return true;
}

}  // namespace instr
}  // namespace v3d
}  // namespace V3DLib
