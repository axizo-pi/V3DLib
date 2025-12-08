#include "Satisfy.h"
#include <assert.h>
#include <stdio.h>
#include "Support/Platform.h"
#include "Target/instr/Mnemonics.h"
#include "Liveness/UseDef.h"
#include "Support/basics.h"
#include "v3d/instr/SmallImm.h"  // float_to_opcode_value()

using namespace Log;
using namespace V3DLib::Target::instr;
using V3DLib::v3d::instr::SmallImm;

namespace V3DLib {

namespace {

bool hasRegFileConflict(Instr const &instr) {
  if (instr.tag == ALU && instr.ALU.srcA.is_reg() && instr.ALU.srcB.is_reg()) {
    int rfa = instr.ALU.srcA.reg().regfile();
    int rfb = instr.ALU.srcB.reg().regfile();

    if (rfa != NONE && rfb != NONE) {
      return (rfa == rfb) && !(instr.ALU.srcA == instr.ALU.srcB);
    }
  }

  return false;
}


/**
 * First pass for satisfy constraints: insert move-to-accumulator instructions
 */
Instr::List insertMoves(Instr::List &instrs) {
  assert(Platform::compiling_for_vc4());  // Not an issue for v3d

  using namespace V3DLib::Target::instr;

  Instr::List newInstrs(instrs.size() * 2);

	Reg acc = ACC0();

  for (int i = 0; i < instrs.size(); i++) {
    using namespace Target::instr;
    Instr instr = instrs[i];

    if (instr.tag == ALU && instr.ALU.srcA.is_imm() &&
        instr.ALU.srcB.is_reg() && instr.ALU.srcB.reg().regfile() == REG_B) {
      // Insert moves for an operation with a small immediate whose
      // register operand must reside in reg file B.
      newInstrs << mov(acc, instr.ALU.srcB)
                << instr.clone().src_b(acc);
    } else if (instr.tag == ALU && instr.ALU.srcB.is_imm() &&
               instr.ALU.srcA.is_reg() && instr.ALU.srcA.reg().regfile() == REG_B) {
      // Insert moves for an operation with a small immediate whose
      // register operand must reside in reg file B.
      newInstrs << mov(acc, instr.ALU.srcA)
                << instr.clone().src_a(acc);
    } else if (hasRegFileConflict(instr)) {
      // Insert moves for operands that are mapped to the same reg file.
      //
      // When an instruction uses two (different) registers that are mapped
      // to the same register file, then remap one of them to an accumulator.
      newInstrs << mov(acc, instr.ALU.srcA)
                << instr.clone().src_a(acc);
    } else {
      newInstrs << instr;
    }
    
  }

  return newInstrs;
}


Instr::List translate_rot(Instr::List &instrs) {
	assert(Platform::compiling_for_vc4());

  Instr::List newInstrs(instrs.size() * 2);

  for (int i = 0; i < instrs.size(); i++) {
    Instr instr = instrs[i];

    if (instr.isRot()) {
      // Insert moves for horizontal rotate operations
      using namespace Target::instr;
	
			Reg acc = ACC0();

      newInstrs << mov(acc, instr.ALU.srcA);

      auto instr2 = instr.clone().src_a(acc);

 	    if (instr.ALU.srcB.is_reg()) {
        newInstrs << mov(ACC5, instr.ALU.srcB);
 	      instr2.src_b(ACC5);
   	  }

      newInstrs << Instr::nop()
 	              << instr2;
 		} else {
      newInstrs << instr;
    }
  }

  return newInstrs;
}


/**
 * Second pass satisfy constraints: insert NOPs
 */
Instr::List insertNops(Instr::List &instrs) {
  Instr::List newInstrs(instrs.size() * 2);


  Instr prev = Instr::nop();

  for (int i = 0; i < instrs.size(); i++) {
    Instr instr = instrs[i];

    // 
    // For vc4, if an rf-register is set, you must wait one cycle before the value is available.
    //
    // If an rf-register is set, and used immediately in the next instruction, insert a NOP in between.
    //
    // v3d does not have this restriction.
    //
    if (Platform::compiling_for_vc4()) {
      Reg dst = prev.dst_reg();
      if (dst.tag != NONE) {
        bool needNop = dst.tag == REG_A || dst.tag == REG_B;  // rf-registers only

        if (needNop && instr.is_src_reg(dst)) {
          newInstrs << Instr::nop();
        }
      }
    }

    newInstrs << instr;                                           // Put current instruction into the new sequence


    // 
    // Insert NOPs in branch delay slots
    //
    if (instr.tag == BRL || instr.tag == END) {
      for (int j = 0; j < 3; j++)
        newInstrs << Instr::nop();

      prev = Instr::nop();
    }

    if (instr.tag != LAB) prev = instr;                           // Update previous instruction
  }

  return newInstrs;
}


/**
 * Insert NOPs between VPM setup and VPM read, if needed
 *
 * Replaces VPM_STALL instructions with NOPs.
 */
Instr::List removeVPMStall(Instr::List &instrs) {
  Instr::List newInstrs(instrs.size() * 2);

  for (int i = 0; i < instrs.size(); i++) {
    Instr instr = instrs[i];

    if (instr.tag != VPM_STALL) {
      newInstrs << instr;
      continue;
    }

    int numNops = 3;  // Number of nops to insert

    for (int j = 1; j <= 3; j++) {
      if ((i + j) >= instrs.size()) break;
      Instr next = instrs[i+j];

      if (next.tag == LAB) break;
      if (next.is_src_reg(Target::instr::VPM_READ)) break;

      numNops--;
    }

    for (int j = 0; j < numNops; j++)
      newInstrs << Instr::nop();
  }

  return newInstrs;
}


/**
 * @param value      output; if conversion succeeds, the value to shift.
 * @param left_shift output; if conversion succeeds, the amount to shift the value with.
 *
 * @return true if conversion succeeds, false otherwise.
 */
bool convert_int_power(int in_value, int &value, int &left_shift) {
  if (in_value < 0)  return false;  // only positive values for now
  if (in_value < 16) return false;  // don't bother with values within range

  value = in_value;
  left_shift = 0;

  while (value != 0 && (value & 1) == 0) {
    left_shift++;
    value >>= 1;
  }

  if (left_shift == 0) return false;
  if (left_shift >= 16) return false;  // Must be positive small int

	return true;
}


bool convert_int_powers(Instr::List &output, Reg dst, int in_value) {
	std::string comment;
	comment << "Load immediate " << in_value;

	//warn << "convert_int_powers in_value: " << in_value;

	if  (-16 <= in_value && in_value <= 15) {
		// No conversion necessary
		output << li(dst, in_value).comment(comment);
		return true;
	}

	int value;
	int left_shift;

	if (!convert_int_power(in_value, value, left_shift)) { 
		//warn << "convert_int_power failed for: " << in_value;


		return false;
	}

	// warn <<  "convert_int_powers " << in_value << " : " << value << " << " << left_shift;
	assert (-16 <= left_shift && left_shift <= 15);

	// 42 = 21 << 1 fails, because 21 is not a small int.
	if (!(-16 <= value && value <= 15)) {
		return false;
	}

	Reg tmp(VarGen::fresh());

	output << li(tmp, value).comment(comment)
         << shl(dst, tmp, left_shift);

	return true;
}


/**
 * Blunt tool for converting all int's.
 *
 * @param output    output parameter, sequence of instructions to
 *                  add generated code to
 * @param in_value  value to encode
 *
 * @return  true if conversion successful, false otherwise
 */
bool encode_int_immediate(Instr::List &output, Reg dst, int in_value) {
  Instr::List ret;
  uint32_t value = (uint32_t) in_value;

	//warn << "Called encode_int_immediate, value: " << hex << value;

  uint32_t nibbles[8];

  for (int i = 0; i < 8; ++i) {
    nibbles[i] = (value  >> (4*i)) & 0xf;
  }

	Reg r0(VarGen::fresh());  // temp value
	Reg r1(VarGen::fresh());  // temp value

  bool did_first = false;
  for (int i = 7; i >= 0; --i) {
    if (nibbles[i] == 0) continue;

    int imm = nibbles[i];

    if (!did_first) {
      ret << mov(dst, imm);  // Gives segfault on p4-3 (arm32)
                            // No clue why, works fine on silke (arm64) and pi3

      if (i > 0) {
        if (convert_int_powers(ret, r0, 4*i)) {
          // r0 now contains value for left shift
          ret << shl(dst, dst, r0).comment("Here 1");
        } else {
          ret << shl(dst, dst, 4*i).comment("Here 2");;
        }
      }
      did_first = true;
    } else {
      if (i > 0) {
        ret << mov(r1, imm);

        if (convert_int_powers(ret, r0, 4*i)) {
          // r0 now contains value for left shift
          ret << shl(r1, r1, r0).comment("Here 3");;
        } else {
          ret << shl(r1, r1, 4*i).comment("Here 4");;
        }

        ret << bor(dst, dst, r1);
      } else {
        ret << bor(dst, dst, imm).comment("Here 5");
      }
    }
  }

  assert(!ret.empty());
  if (ret.empty()) return false;  // Not expected, but you never know

  std::string cmt;
  cmt << "Full load imm " << in_value;
  ret.front().comment(cmt);

  std::string cmt2;
  cmt2 << "full load imm " << in_value;
  ret.back().comment(cmt2);

  output << ret;
  return true;
}


bool can_encode(Imm const &imm) {
  int rep_value;

  switch (imm.tag()) {
  case Imm::IMM_INT32: {
    int value = imm.intVal();
  	if (SmallImm::int_to_opcode_value(value, rep_value)) return true;
	}
  break;

  case Imm::IMM_FLOAT32: {
    float value = imm.floatVal();
  	if (SmallImm::float_to_opcode_value(value, rep_value)) return true;
	}
  break;

  case Imm::IMM_MASK:
    debug_break("can_encode: IMM_MASK not handled");
  break;

  default:
    debug_break("can_encode: unknown tag value");
  break;
	}

	return false;
}



bool encode_int(Instr::List &ret, Reg dst, int value) {
	Instr::List tmp;

	//warn << "encode_int value: " << value;

  if (convert_int_powers(tmp, dst, value)) {           // powers of 2 of basic small int's 
  } else if (encode_int_immediate(tmp, dst, value)) {  // Use full blunt conversion (heavy but always works)
  } else {
    return false;                                      // Conversion failed
	}

	//warn << "encode_int ret:\n" << tmp.dump();
	ret << tmp;
  return true;
}


bool encode_float(Instr::List &ret, Reg dst, float value) {
	Instr::List tmp;
  int rep_value;

	// Do known negative small imm values
  if (value < 0 && SmallImm::float_to_opcode_value(-value, rep_value)) {
    std::string cmt;
    cmt << "Load neg float small imm " << value;

		// Small Imm conversion happens downstream
		tmp << li(dst, -value).comment(cmt)
        << fsub(dst, 0, dst);                  // Works because float zero is 0x0

		ret << tmp;
  	return true;
  }

	// Try to convert from int
	if ((float) ((int) value) == value) {
  	if (SmallImm::int_to_opcode_value((int) value, rep_value)) {
			warn << "encode_float: can load " << value << " as integer";
    
			std::string cmt;
    	cmt << "Load float from int " << value;

			tmp << li(dst, (int) value).comment(cmt)
      	  << itof(dst, dst);

			ret << tmp;
	  	return true;
		}
	}
	

  // Do the full blunt int conversion
	Reg r1(VarGen::fresh());  // temp value

  int int_value = *((int *) &value);
  if (encode_int_immediate(ret, r1, int_value)) {
    std::string cmt;
    cmt << "Load full float imm " << value;

    tmp << mov(dst, r1);          // Result is int but will be handled as float downstream
	  tmp.front().comment(cmt);

	  ret << tmp;
  	return true;
  }

  return false;
}


Instr::List _encode_imm(Reg dst, Imm imm) {
	assert(!can_encode(imm));

	Instr::List ret;

  std::string err_label;
  std::string err_value;


  if (imm.tag() == Imm::IMM_INT32) {
    int value = imm.intVal();

    if (!encode_int(ret, dst, value)) {
      // Conversion failed, output error
      err_label = "int";
      err_value = std::to_string(value);
    }
  } else {
  	assert(imm.tag() == Imm::IMM_FLOAT32);

    float value = imm.floatVal();
  	//warn << "Handling case Imm::IMM_FLOAT32, value: " << value;

    if (!encode_float(ret, dst, value)) {
      // Conversion failed, output error
      err_label = "float";
      err_value = std::to_string(value);
    }
  }

  if (!err_value.empty()) {
    assert(!err_label.empty());

    cerr << "LI: Can't handle "
         << err_label << " value '" << err_value
         << "' as small immediate";

    breakpoint
  }

  return ret;
}

}  // anon namespace


Instr::List encode_imm(V3DLib::Instr const &instr) {
  Instr::List ret;

	bool do_li = false;
	Imm imm;
	Reg dst;

  if (instr.tag == LI) {
		do_li = true;
  	imm = instr.LI.imm;
  	dst  = instr.dest();
	} else {
    int num_ops = instr.ALU.num_operands();

    if (num_ops == 0) {
			ret << instr;
			return ret;
		}

    if (num_ops == 2) {
			if (instr.ALU.srcA.is_imm() && instr.ALU.srcB.is_imm()) {
				cerr << "encode_imm: not handling case of two imm operands yet" << thrw;
			}

			ret << instr;
			return ret;
		}

		if (!instr.ALU.srcA.is_imm()) { 
			ret << instr;
			return ret;
		} else {
			do_li = false;
  		imm = instr.ALU.srcA.imm();
  		dst = VarGen::fresh();
		}
	}

	if (can_encode(imm)) {
		// Will be handled correctly downstream
		ret << instr;
		return ret;
	}

  ret = _encode_imm(dst, imm);
	assert(!ret.empty());

	if (ret.size() == 1 && ret[0] == instr) {
		// No conversion
		breakpoint;  // Shouldn't really happen, warn me
		return ret;
	}

	if (!do_li) {
		auto new_instr = instr;
		new_instr.ALU.srcA = dst;
		ret << new_instr;
	}


  if (instr.set_cond().flags_set()) {
    breakpoint;  // to check what flags need to be set - case not handled yet
  }

	for (int i = 0; i < ret.size(); ++i) {
	  ret.get(i).cond(instr.assign_cond());
	}

	return ret;
}


void adjust_immediates(Instr::List &instrs) {
  assert(!Platform::compiling_for_vc4());  // v3d only

	Instr::List res;

  for (int i = 0; i < instrs.size(); i++) {
		Instr const &instr = instrs[i];

		if ( instr.tag == LI || instr.tag == ALU) {
			res << encode_imm(instr);
		} else {
			res << instr;
		}
	}

	instrs = res;
}



/**
 * Satisfy VideoCore constraints
 *
 * Transform an instruction sequence to satisfy various VideoCore
 * constraints, including:
 *
 *   1. fill branch delay slots with NOPs;
 *
 *   2. introduce accumulators for operands mapped to the same
 *      register file;
 *
 *   3. introduce accumulators for horizontal rotation operands;
 *
 *   4. insert NOPs to account for data hazards: a destination
 *      register (assuming it's not an accumulator) cannot be read by
 *      the next instruction.
 */
void vc4_satisfy(Instr::List &instrs) {
  Instr::List newInstrs = translate_rot(instrs);

  newInstrs = insertMoves(newInstrs);
  newInstrs = insertNops(newInstrs);
  instrs = removeVPMStall(newInstrs);
}


/**
 * See header comment vc4_satisfy()
 */
void v3d_satisfy(Instr::List &instrs) {
  Instr::List newInstrs = instrs;

  newInstrs = insertNops(newInstrs);
  instrs = removeVPMStall(newInstrs);
}

}  // namespace V3DLib
