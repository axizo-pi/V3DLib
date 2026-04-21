#include "OpItems.h"
#include <vector>
#include "Support/basics.h"
#include "Support/Platform.h"

namespace V3DLib {
namespace {

std::vector<OpItem> m_list = {
  {ADD,    "+",        false, Enum::A_FADD,   Enum::A_ADD},
  {SUB,    "-",        false, Enum::A_FSUB,   Enum::A_SUB},
  {MUL,    "*",        false, Enum::M_FMUL,   Enum::M_MUL24},
  {MIN,    " min ",    false, Enum::A_FMIN,   Enum::A_MIN},
  {MAX,    " max ",    false, Enum::A_FMAX,   Enum::A_MAX},
  {ItoF,   "(Float) ", false, Enum::A_ItoF,   Enum::OP_UNDEFINED,    false, 1},
  {FtoI,   "(Int) ",   false, Enum::OP_UNDEFINED,     Enum::A_FtoI,  false, 1},
  {ROTATE, " rotate ", false, Enum::M_ROTATE, Enum::M_ROTATE},
  {SHL,    " << ",     false, Enum::OP_UNDEFINED,     Enum::A_SHL},
  {SHR,    " >> ",     false, Enum::OP_UNDEFINED,     Enum::A_ASR},
  {USHR,   " _>> ",    false, Enum::OP_UNDEFINED,     Enum::A_SHR},
  {ROR,    " ror ",    false, Enum::OP_UNDEFINED,     Enum::A_ROR},
  {BAND,   " & ",      false, Enum::OP_UNDEFINED,     Enum::A_BAND},
  {BOR,    " | ",      false, Enum::OP_UNDEFINED,     Enum::A_BOR},
  {BXOR,   " ^ ",      false, Enum::OP_UNDEFINED,     Enum::A_BXOR},
  {BNOT,   "~",        false, Enum::OP_UNDEFINED,     Enum::A_BNOT, false, 1},

  // v3d-specific
  {FFLOOR, "ffloor",    true, Enum::A_FFLOOR, Enum::OP_UNDEFINED,   true, 1},
  {SIN,    "sin",       true, Enum::A_FSIN,   Enum::OP_UNDEFINED,   true, 1},  // also SFU function
  {TIDX,   "tidx",     false, Enum::OP_UNDEFINED,     Enum::A_TIDX, true, 0},
  {EIDX,   "eidx",     false, Enum::OP_UNDEFINED,     Enum::A_EIDX, true, 0},
  {BARRIER,"barrier",  false, Enum::OP_UNDEFINED,     Enum::A_BARRIER, true, 0},

  // SFU functions
  {RECIP,     "recip",     true, Enum::A_RECIP, Enum::OP_UNDEFINED, false, 1},
  {RECIPSQRT, "recipsqrt", true, Enum::A_RSQRT, Enum::OP_UNDEFINED, false, 1},
  {EXP,       "exp",       true, Enum::A_EXP,   Enum::OP_UNDEFINED, false, 1},
  {LOG,       "log",       true, Enum::A_LOG,   Enum::OP_UNDEFINED, false, 1},

  // Derived instructions
  {EXP_E,     "exp_e",     true, Enum::OP_UNDEFINED,    Enum::OP_UNDEFINED, false, 1},
  {TANH,      "tanh",      true, Enum::OP_UNDEFINED,    Enum::OP_UNDEFINED, false, 1},

  //
  // Following are not direct instructions in the source language
  //
  {_DUMMY,    "",         false, Enum::A_TMUWT, Enum::OP_UNDEFINED, false, 0},
  {_DUMMY,    "",         false, Enum::A_TIDX,  Enum::OP_UNDEFINED, false, 0},
  {_DUMMY,    "",         false, Enum::A_EIDX,  Enum::OP_UNDEFINED, false, 0},
  {_DUMMY,    "",         false, Enum::A_MOV,   Enum::OP_UNDEFINED, false, 1},
};

}  // anon namespace

///////////////////////////////////////////////////////////////////////////////
// Class OpItem
///////////////////////////////////////////////////////////////////////////////

OpItem::OpItem(
  OpId in_tag,
  char const *in_str,
  bool in_is_function,
  Enum in_aluop_float,
  Enum in_aluop_int,
  bool in_v3d_specific,
  int in_num_params
) :
  tag(in_tag),
  str(in_str),
  is_function(in_is_function),
  m_aluop_float(in_aluop_float),
  m_aluop_int(in_aluop_int),
  m_num_params(in_is_function?1:in_num_params),
  m_v3d_specific(in_v3d_specific)
{}


int OpItem::num_params() const {
  assertq(!is_function || m_num_params == 1, "Function op should always have 1 parameter");
  return m_num_params;
}


Enum OpItem::aluop_float() const {
  return m_aluop_float;
}


Enum OpItem::aluop_int() const {
  return m_aluop_int;
}


std::string OpItem::disp(std::string const &lhs, std::string const &rhs) const {
  std::string ret;

  if (num_params() == 0) {
    ret << str << "()";
  } else if (is_function) {
    assert(!lhs.empty());
    ret << str << "(" << lhs << ")";
  } else if (num_params() == 1) {
    assert(!lhs.empty());
    ret << "(" << str << lhs << ")";
  } else {
    assert(num_params() == 2);
    assert(!lhs.empty());
    assert(!rhs.empty());
    ret << "(" << lhs << str << rhs <<  ")";
  }

  return ret;
}


std::string OpItem::dump() const {
  std::string ret;
  ret << "Op " << tag << " (" << str << ")";
  return ret;
}


namespace {

OpItem const *find(OpId id) {
  for (auto &it : m_list) {
    if (it.tag == id) return &it;
  }
  return nullptr;
}

} // anon namespace


namespace OpItems {

OpItem const &get(OpId id) {
  OpItem const *item = find(id);
  assert(item != nullptr);
  return *item;
}


/**
 * Translate source operator to target opcode
 */
Enum opcode(Op const &op) {
  auto const *item = find(op.op);

  if (item == nullptr) {
    if (op.type == BaseType::FLOAT) {
      assertq(false, "opcode(): Unhandled op for float");
    } else {
      assertq(false, "opcode(): Unhandled op for int");
    }
    return Enum::NOP;
  }

  if (item->v3d_specific()) {
    if (Platform::compiling_for_vc4()) {
      std::string msg;
      msg << "opcode(): " << item->dump() << " is only for v3d";
      assertq(msg);
    }
  }

  if (op.type == BaseType::FLOAT) {
    return item->aluop_float();
  } else {
    return item->aluop_int();
  }
}


int num_operands_by_op(Enum op) {
  assert(op != Enum::OP_UNDEFINED);
  assert(op != Enum::NOP);

  int num = -1;
  for (auto &it : m_list) {
    if (it.aluop_float() == op) { num = it.num_params(); break; }
    if (it.aluop_int()   == op) { num = it.num_params(); break; }
  }

  if (num == -1) {
    breakpoint;
  }
  return num;
}

} // namespace OpItems

}  // namespace V3DLib

