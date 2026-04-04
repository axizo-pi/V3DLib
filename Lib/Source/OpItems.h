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
    Enum in_aluop_float,
    Enum in_aluop_int,
    bool in_v3d_specific = false,
    int in_num_params = 2
  );

  OpId tag;
  char const *str;
  bool is_function;

  int num_params() const;
  Enum aluop_float() const;
  Enum aluop_int() const;
  bool v3d_specific() const { return m_v3d_specific; }

  std::string disp(std::string const &lhs, std::string const &rhs) const;
  std::string dump() const;

private:
  Enum m_aluop_float;
  Enum m_aluop_int;
  int  m_num_params   = -1;
  bool m_v3d_specific = false;
};


namespace OpItems {

Enum opcode(Op const &op);
OpItem const &get(OpId id);
int num_operands_by_op(Enum op);

} // namespace OpItems


}  // namespace V3DLib

#endif  // _V3DLIB_SOURCE_OPITEMS_H_
