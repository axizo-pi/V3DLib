#include "ALUOp.h"
#include <stdint.h>
#include "Support/basics.h"
#include "Source/Op.h"

using namespace Log;

namespace V3DLib {

op_item::op_item(ALUOp::Enum in_op, v3d_qpu_add_op in_add_op) :
  op(in_op),
  has_add_op(true),
  add_op(in_add_op)
{}


op_item::op_item(ALUOp::Enum in_op, bool in_add_op, v3d_qpu_mul_op in_mul_op) :
  op(in_op),
  has_mul_op(true),
  mul_op(in_mul_op)
{
  assert(!in_add_op);
}


op_item::op_item(ALUOp::Enum in_op, v3d_qpu_add_op in_add_op, v3d_qpu_mul_op in_mul_op) :
  op(in_op),
  has_add_op(true),
  add_op(in_add_op),
  has_mul_op(true),
  mul_op(in_mul_op)
{}


namespace {

std::vector<op_item> op_items = {
  { ALUOp::A_FADD,   V3D_QPU_A_FADD },  // NOTE: ADD on mul alu is int only
  { ALUOp::A_FSUB,   V3D_QPU_A_FSUB },  //       SUB on mul alu is int only
  { ALUOp::A_FtoI,   V3D_QPU_A_FTOIN  },
  { ALUOp::A_ItoF,   V3D_QPU_A_ITOF   },
  { ALUOp::A_ADD,    V3D_QPU_A_ADD,   V3D_QPU_M_ADD },
  { ALUOp::A_SUB,    V3D_QPU_A_SUB,   V3D_QPU_M_SUB },
  { ALUOp::A_SHR,    V3D_QPU_A_SHR    },
  { ALUOp::A_ASR,    V3D_QPU_A_ASR    },
  { ALUOp::A_SHL,    V3D_QPU_A_SHL    },
  { ALUOp::A_MIN,    V3D_QPU_A_MIN    },
  { ALUOp::A_MAX,    V3D_QPU_A_MAX    },
  { ALUOp::A_BAND,   V3D_QPU_A_AND    },
  { ALUOp::A_BOR,    V3D_QPU_A_OR     },
  { ALUOp::A_BXOR,   V3D_QPU_A_XOR    },
  { ALUOp::A_BNOT,   V3D_QPU_A_NOT    },
  { ALUOp::M_FMUL,   false,           V3D_QPU_M_FMUL },
  { ALUOp::M_MUL24,  false,           V3D_QPU_M_SMUL24 },
  { ALUOp::M_ROTATE, false,           V3D_QPU_M_MOV },     // < vc7:Special case: it's a mul alu mov with sig.rotate set
  { ALUOp::A_TIDX,   V3D_QPU_A_TIDX   },
  { ALUOp::A_EIDX,   V3D_QPU_A_EIDX   },
  { ALUOp::A_FFLOOR, V3D_QPU_A_FFLOOR },
  { ALUOp::A_FSIN,   V3D_QPU_A_SIN    },                   // NOTE: Extra NOP's and read in generation
  { ALUOp::A_TMUWT,  V3D_QPU_A_TMUWT  },                   // NOTE: Extra NOP's and read in generation

  // v3d
  { ALUOp::A_FMOV,   V3D_QPU_A_FMOV   },

	// VC7
  { ALUOp::A_MOV,    V3D_QPU_A_MOV    },
  { ALUOp::A_EXP,    V3D_QPU_A_EXP    },
  { ALUOp::A_RECIP,  V3D_QPU_A_RECIP  },
};


void op_items_check_sorted() {
  static bool checked = false;

  if (checked) return;

  bool did_first = false;
  ALUOp::Enum previous;

  for (auto const &item : op_items) {
    if (!did_first) {
      previous = item.op;
      did_first = true;
      continue;
    }

    assertq(previous < item.op, "op_items not sorted on (target) op");
    previous = item.op;
  }

  checked = true;
}


/**
 * Derived from (iterative version): https://iq.opengenus.org/binary-search-in-cpp/
 */
int op_items_binary_search(int left, int right, ALUOp::Enum needle) {
  while (left <= right) { 
    int middle = (left + right) / 2; 

    if (op_items[middle].op == needle) 
      return middle;  // found it

    // If element is greater, ignore left half 
    if (op_items[middle].op < needle) 
      left = middle + 1; 

    // If element is smaller, ignore right half 
    else
      right = middle - 1; 
  } 

  return -1; // element not found
}

} // anon namespace

ALUOp::ALUOp(Op const &op) : m_value(op.opcode()) {}


/**
 * Determine if current is a mul-ALU instruction
    bool uncond = instr.tag == BRL && instr.branch_cond().tag == COND_ALWAYS;
 * Determines if the mul-ALU needs to be used
 *
 * TODO: Examine if this is still true for v3d
 */
bool ALUOp::isMul() const {
  return (M_FMUL <= m_value && m_value <= M_ROTATE);
}


std::string ALUOp::pretty() const {
  switch (m_value) {
    case NOP:       return "nop";
    case A_FADD:    return "addf";
    case A_FSUB:    return "subf";
    case A_FMIN:    return "minf";
    case A_FMAX:    return "maxf";
    case A_FMINABS: return "minabsf";
    case A_FMAXABS: return "maxabsf";
    case A_FtoI:    return "ftoi";
    case A_ItoF:    return "itof";
    case A_ADD:     return "add";
    case A_SUB:     return "sub";
    case A_SHR:     return "shr";
    case A_ASR:     return "asr";
    case A_ROR:     return "ror";
    case A_SHL:     return "shl";
    case A_MIN:     return "min";
    case A_MAX:     return "max";
    case A_BAND:    return "and";
    case A_BOR:     return "or";
    case A_BXOR:    return "xor";
    case A_BNOT:    return "not";
    case A_CLZ:     return "clz";
    case A_V8ADDS:  return "addsatb";
    case A_V8SUBS:  return "subsatb";
    case M_FMUL:    return "fmul";
    case M_MUL24:   return "mul24";
    case M_V8MUL:   return "mulb";
    case M_V8MIN:   return "minb";
    case M_V8MAX:   return "maxb";
    case M_V8ADDS:  return "m_addsatb";
    case M_V8SUBS:  return "m_subsatb";
    case M_ROTATE:  return "rotate";

    // v3d-specific
    case A_TIDX:    return "tidx";
    case A_EIDX:    return "eidx";
    case A_FFLOOR:  return "ffloor";
    case A_FSIN:    return "sin";
    case A_TMUWT:   return "tmuwt";
    case A_FMOV:    return "ifmov";

		// vc7
    case A_MOV:     return "mov";
    case A_EXP:     return "exp";
    case A_RECIP:   return "recip";

    default:
      assertq(false, "pretty(): Unknown ALU opcode", true);
      return "";
  }
}


uint32_t ALUOp::vc4_encodeAddOp() const {

/////////////////////////////////////////////////////////
// Here is the reason vc4 wasn't working!!!!
/////////////////////////////////////////////////////////

/* <<<<<<<<<<<<<<<<<<<<<<<<
  if (NOP <= m_value && m_value <= A_V8SUBS) return m_value;
   ======================== */
  switch (m_value) {
    case NOP:       return 0;
    case A_FADD:    return 1;
    case A_FSUB:    return 2;
    case A_FMIN:    return 3;
    case A_FMAX:    return 4;
    case A_FMINABS: return 5;
    case A_FMAXABS: return 6;
    case A_FtoI:    return 7;
    case A_ItoF:    return 8;
    case A_ADD:     return 12;
    case A_SUB:     return 13;
    case A_SHR:     return 14;
    case A_ASR:     return 15;
    case A_ROR:     return 16;
    case A_SHL:     return 17;
    case A_MIN:     return 18;
    case A_MAX:     return 19;
    case A_BAND:    return 20;
    case A_BOR:     return 21;
    case A_BXOR:    return 22;
    case A_BNOT:    return 23;
    case A_CLZ:     return 24;
    case A_V8ADDS:  return 30;
    case A_V8SUBS:  return 31;
/* >>>>>>>>>>>>>>>>>>>>>>>> */

/* <<<<<<<<<<<<<<<<<<<<<<<<
  assertq("V3DLib: unknown add op", true);
  return 0;
   ======================== */
    default:
      assertq("V3DLib: unknown add op", true);
      return 0;
  }
/* >>>>>>>>>>>>>>>>>>>>>>>> */
}


uint32_t ALUOp::vc4_encodeMulOp() const {

/////////////////////////////////////////////////////////
// Here is the second reason vc4 wasn't working!!!!
/////////////////////////////////////////////////////////

/* <<<<<<<<<<<<<<<<<<<<<<<<
  if (m_value == NOP) return NOP;
  if (isMul() && m_value != M_ROTATE) return m_value - M_FMUL; 
   ======================== */
  switch (m_value) {
    case NOP:      return 0;
    case M_FMUL:   return 1;
    case M_MUL24:  return 2;
    case M_V8MUL:  return 3;
    case M_V8MIN:  return 4;
    case M_V8MAX:  return 5;
    case M_V8ADDS: return 6;
    case M_V8SUBS: return 7;
/* >>>>>>>>>>>>>>>>>>>>>>>> */

/* <<<<<<<<<<<<<<<<<<<<<<<<
  fatal("V3DLib: unknown MUL op");
  return 0;
   ======================== */
    default:
      fatal("V3DLib: unknown mul op");
      return 0;
  }
/* >>>>>>>>>>>>>>>>>>>>>>>> */
}


op_item const *op_items_find_by_op(ALUOp::Enum op, bool strict) {
  op_items_check_sorted();

  int index = op_items_binary_search(0, (int) op_items.size() - 1, op);

  if (index == -1) {
    if (strict) {
      cerr << "Could not find item for op: " << op << thrw;
    }
    return nullptr;
  }

  return &op_items[index];
}


namespace Oper {

// TODO Consolidate this with next call
bool oneOperand(v3d_qpu_add_op op) {
  return (       // these should be the only operations with one operand
	  op == V3D_QPU_A_SIN   ||
		op == V3D_QPU_A_FFLOOR ||

    // v3d
		op == V3D_QPU_A_FMOV    ||

		// vc7 
		op == V3D_QPU_A_MOV    ||
		op == V3D_QPU_A_EXP    ||
		op == V3D_QPU_A_RECIP
	);
}


bool oneOperand(ALUOp const &op) {
  return (       // these should be the only operations with one operand
	  op.value() == ALUOp::A_FSIN   ||
		op.value() == ALUOp::A_FFLOOR ||

    // v3d
		op.value() == ALUOp::A_FMOV    ||

		// vc7 
		op.value() == ALUOp::A_MOV    ||
		op.value() == ALUOp::A_EXP    ||
		op.value() == ALUOp::A_RECIP
	);
}


bool noOperands(ALUOp const &op) {
  return (        // These should be the only operations with no operands
	  op.value() == ALUOp::A_TMUWT ||
		op.value() == ALUOp::A_TIDX  ||
		op.value() == ALUOp::A_EIDX
  );
}

} // namespace Oper

}  // namespace V3DLib
