#ifndef _V3DLIB_SOURCE_OPITEMS_H_
#define _V3DLIB_SOURCE_OPITEMS_H_
#include "Op.h"
#include "Target/instr/ALUOp.h"

namespace V3DLib {

struct OpItem {
  OpItem(
    OpId in_tag,
    char const *in_str,
    bool in_is_function,
    ALUOp::Enum in_aluop_float,
    ALUOp::Enum in_aluop_int,
    bool in_v3d_specific = false,
    int in_num_params = 2
  );

  OpId tag;
  char const *str;
  bool is_function;

  int num_params() const;
  ALUOp::Enum aluop_float() const;
  ALUOp::Enum aluop_int() const;
  bool v3d_specific() const { return m_v3d_specific; }

  std::string disp(std::string const &lhs, std::string const &rhs) const;
  std::string dump() const;

private:
  ALUOp::Enum m_aluop_float;
  ALUOp::Enum m_aluop_int;
  int  m_num_params   = -1;
  bool m_v3d_specific = false;
};


namespace OpItems {

ALUOp::Enum opcode(Op const &op);
OpItem const &get(OpId id);
int num_operands_by_op(ALUOp::Enum op);

} // namespace OpItems


}  // namespace V3DLib

#endif  // _V3DLIB_SOURCE_OPITEMS_H_
