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


/**
 * Combination only possible if instructions not both add ALU or both mul ALU
 *
 * TODO Not used any more (20210614), check if should be removed
 */
bool OpItems::valid_combine_pair(Instr const &instr, Instr const &next_instr, bool &do_converse) {
  if (uses_add_alu(instr) && uses_mul_alu(next_instr)) {
    do_converse = false;
    return true;
  }

  if (uses_mul_alu(instr) && uses_add_alu(next_instr)) {
    do_converse = true;
    return true;
  }

  return false;
}

}  // namespace instr
}  // namespace v3d
}  // namespace V3DLib
