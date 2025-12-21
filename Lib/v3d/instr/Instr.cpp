///////////////////////////////////////////////////////////////////////////////
// Opcodes for v3d
//
///////////////////////////////////////////////////////////////////////////////
#include "Instr.h"
#include <cstdio>
#include <cstdlib>        // abs()
#include <bits/stdc++.h>  // swap()
#include "Support/basics.h"
#include "Mnemonics.h"
#include "OpItems.h"
#include "Support/Platform.h"

using namespace Log;

namespace V3DLib {
namespace v3d {

using ::operator<<;  // C++ weirdness; come on c++, get a grip.

namespace {

#ifdef DEBUG
std::string binaryValue(uint64_t num) {
  const int size = sizeof(num)*8;
  std::string result; 

  for (int i = size -1; i >=0; i--) {
    bool val = ((num >> i) & 1) == 1;

    result += val?'1':'0';

    if (i % 10 == 0) {
      result += '.';
    }
  }

  return result;
}
#endif


v3d_qpu_cond translate_assign_cond(AssignCond cond) {
  assertq(cond.tag != AssignCond::Tag::NEVER, "Not expecting NEVER (yet)", true);

  v3d_qpu_cond tag_value = V3D_QPU_COND_NONE;

  if (cond.tag != AssignCond::Tag::FLAG) return tag_value;

  switch(cond.flag) {
    case ZS:
    case NS:
      // set ifa tag
      tag_value = V3D_QPU_COND_IFA;
      break;
    case ZC:
    case NC:
      // set ifna tag
      tag_value = V3D_QPU_COND_IFNA;
      break;
    default: break;
  }

  return tag_value;
}

}  // anon namespace

namespace instr {

namespace {

MAYBE_UNUSED std::string dump_check(CheckSrc val) {
	std::string ret;

	switch (val) {
  	case CHECK_ADD_A: ret << "addr.a"; break;
  	case CHECK_ADD_B: ret << "addr.b"; break;
  	case CHECK_MUL_A: ret << "mul.a"; break;
		case CHECK_MUL_B: ret << "mul.b"; break;
	}

	return ret;
}


bool check_small_imm(Instr const &dst, BaseSource const &src) {
	assert(!Platform::compiling_for_vc7());

	if (dst.sig.small_imm_b) {
		// If the src value is the same as the local small int, this is fine
		return (dst.raddr_b == src.val());
	} else {
		// For the time being, assume OK
		return true;
	}
}


/**
 * Note that the mul a can set the small imm before the mul b does it.
 */
void set_mul_small_imm(
	Instr &dst,
	BaseSource const &imm,
	v3d_qpu_input &input,
	CheckSrc check_src
) {
	assert(!Platform::compiling_for_vc7());
	assert(imm.is_small_imm());

	if (dst.sig.small_imm_b) {
		// If same value, all is well
		assertq(dst.raddr_b == imm.val(), "alu_mul_b: small imm different value on vc6");
		input.mux = V3D_QPU_MUX_B;
	} else {
		// Reserve the small imm
   	if (dst.mux_in_use(check_src, V3D_QPU_MUX_B)) {
			assertq(false, "MUX_B in use for non-small_imm");
		}

		dst.sig.small_imm_b = true;
		input.mux = V3D_QPU_MUX_B;
		dst.raddr_b = imm.val();
		input.unpack = imm.unpack();
	}
}

}  // anon namespace


uint64_t Instr::NOP() {
  if (devinfo_ver() == 42) {
		return 0x3c003186bb800000;  // This is actually 'nop nop'
	} else {
		// 7x, vc7
		return 0x38000000bb03f000;
	}
}


Instr::Instr() {
  init(NOP());
}

Instr::Instr(uint64_t in_code) {
  init(in_code);
	m_external_init = true;
}


bool Instr::is_branch() const {
  return (type == V3D_QPU_INSTR_TYPE_BRANCH);
}


/**
 * Return true if any of the small_imm flags are set (there are four).
 */
bool Instr::has_small_imm() const {
	return sig.small_imm_a  // This is actually vc7. Problem?
	    || sig.small_imm_b
	    || sig.small_imm_c
	    || sig.small_imm_d;
}


/**
 * Determine if there are any specials signals used in this instruction
 *
 * @param all_signals  if true, also flag small_imm and rotate; these have a place in the instructions
 */
bool Instr::has_signal(bool all_signals) const {
  if (all_signals && (has_small_imm() || sig.rotate)) {
    return true;
  }

  if (uses_sig_dst()) return true;

  return (sig.thrsw || sig.ldunif || sig.ldunifa 
       || sig.ldvpm || sig.ucb || sig.wrtmuc);
}


/**
 * Determine if singals use signal destination field.
 *
 * At most one signal should be using it
 * Source: mesa/src/broadcom/qpu/qpu_instr.c v3d_qpu_sig_writes_address()
 */
int Instr::sig_dst_count() const {
  int ld_count = 0;
  if (sig.ldunifrf) ld_count++;
  if (sig.ldunifarf) ld_count++;
  if (sig.ldvary) ld_count++;
  if (sig.ldtmu) ld_count++;
  if (sig.ldtlb) ld_count++;
  if (sig.ldtlbu) ld_count++;

  return ld_count;
}


bool Instr::uses_sig_dst() const {
  int ld_count = sig_dst_count();
  assert(ld_count <= 1);
  return ld_count != 0;
}


bool Instr::flag_push_set() const { return (flags.apf || flags.mpf); }
bool Instr::flag_cond_set() const { return (flags.ac  || flags.mc ); }
bool Instr::flag_uf_set()   const { return (flags.auf || flags.muf); }


/**
 * Set the condition tags during translation.
 *
 * Either add or mul alu condition tags are set here, both not allowed (intentionally too strict condition)
 *
 * Note that mul alu condition tags may be set beforehand, this is accounted for in logic.
 */
void Instr::set_cond_tag(AssignCond cond) {
  assert(!is_branch());
  if (cond.is_always()) return;
  if (add_nop() && mul_nop()) return;  // Don't bother with a full nop instruction

  assertq(cond.tag != AssignCond::Tag::NEVER, "Not expecting NEVER (yet)", true);
  assertq(cond.tag == AssignCond::Tag::FLAG,  "const.tag can only be FLAG here");  // The only remaining option

  v3d_qpu_cond tag_value = translate_assign_cond(cond);
  assert(tag_value != V3D_QPU_COND_NONE);

  assertq(add_nop() || mul_nop(), "Not expecting both add and mul alu to be used", true); 

  if (alu.add.op != V3D_QPU_A_NOP) {
    if (flags.ac == V3D_QPU_COND_NONE) {
      flags.ac = tag_value;
    } else {
      assertq(flags.ac == tag_value, "add alu assign tag already set to different value", true);
    }
  }

  if (alu.mul.op != V3D_QPU_M_NOP) {
    if (flags.mc == V3D_QPU_COND_NONE) {
      flags.mc = tag_value;
    } else {
      assertq(flags.mc == tag_value, "mul alu assign tag already set to different value", true);
    }
  }
}


void Instr::set_push_tag(SetCond set_cond) {
  if (set_cond.tag() == SetCond::NO_COND) return;
  assertq(flags.apf == V3D_QPU_PF_NONE, "Not expecting add alu push tag to be set");
  assertq(flags.mpf == V3D_QPU_PF_NONE, "Not expecting mul alu push tag to be set");
  assertq(set_cond.tag() == SetCond::Z || set_cond.tag() == SetCond::N, "Unhandled SetCond flag", true);

  v3d_qpu_pf tag_value;

  if (set_cond.tag() == SetCond::Z) {
    tag_value = V3D_QPU_PF_PUSHZ;
  } else {
    tag_value = V3D_QPU_PF_PUSHN;
  }

  if (alu.add.op != V3D_QPU_A_NOP) {
    flags.apf = tag_value;
  }

  if (alu.mul.op != V3D_QPU_M_NOP) {
    flags.mpf = tag_value;
  }
}


/**
 * Check if there are same target dst registers
 *
 * Also checks signal dst if present.
 *
 * NOTE: There is no check here is add/mul dst are actually used.
 */
bool Instr::check_dst() const {
  int ld_count = sig_dst_count();
  if (ld_count > 1) {
    cerr << "More than one ld-signal with dst register set";
    return false;
  }

  bool ret = true;

  DestReg sig_dst = sig_dest();
  DestReg add_dst = add_dest();
  DestReg mul_dst = mul_dest();

  if (sig_dst == add_dst) {
    breakpoint
    cerr << "signal dst register same as add alu dst register";
    ret = false;
  }

  if (sig_dst == mul_dst) {
    breakpoint
    cerr << "signal dst register same as mul alu dst register";
    ret = false;
  }

  if (add_dst == mul_dst) {
    breakpoint
    cerr << "add alu dst register same as mul alu dst register";
    ret = false;
  }

  return ret;
}


std::string Instr::dump_internal() const {
/*
	Log::debug
		<< "output pack values:\n"
    << "  add: " << alu.add.output_pack << "\n"
    << "  mul: " << alu.mul.output_pack
	;
*/

  std::string ret = instr_mnemonic(this);

  auto indent = [] (int size) -> std::string {
    const int TAB_POS = 42;  // tab position for signal in mnemonic
    size++;                  // Fudging to get position right
    std::string ret;

    while (size < TAB_POS) {
      ret << " ";
      size++;
    }

    return ret;
  };


  // Output rotate signal (not done in MESA)
  if (sig.rotate) {
    if (!is_branch()) {  // Assumption: rotate signal irrelevant for branch

			if (Platform::compiling_for_vc7()) {
				// Ref. see: mesa2/src/broadcom/qpu/qpu_instr.h
				assert("Don't know how to deal with vc7");
			} else {
	      // Only two possibilities here: r5 or small imm (sig for small imm not set!)
	      if (alu.mul.b.mux == V3D_QPU_MUX_R5) {
	        ret << ", r5";
	      } else if (alu.mul.b.mux == V3D_QPU_MUX_B) {
	        ret << ", " << raddr_b;
	      } else {
	        assertq(false, "dump_internal(): unexpected mux value for mul b for rotate", true);
	      }
			}

      ret << indent((int) ret.size()) << "; rot";
    }
  }

  return ret;
}


std::string Instr::mnemonic(bool with_comments) const {
  std::string ret;

  if (with_comments && !InstructionComment::header().empty()) {
    ret << emit_header();
  }

  std::string out = dump_internal();
  ret << out;

  if (with_comments && !InstructionComment::comment().empty()) {
    ret << emit_comment((int) out.size());
  }

  return ret;
}


uint64_t Instr::bytecode() const {
  uint64_t repack = instr_pack(const_cast<Instr *>(this));
  return repack;
}


std::string Instr::mnemonic(uint64_t in_code) {
  Instr instr(in_code);
  return instr.mnemonic();
}


std::string Instr::mnemonics(std::vector<uint64_t> const &in_code) {
  std::string ret;

  for (int i = 0; i < (int) in_code.size(); i++) {
    ret << i << ": " << mnemonic(in_code[i]) << "\n";
  }

  return ret;
}


void Instr::init(uint64_t in_code) {
  raddr_a = 0;

  // These do not always get initialized in unpack
  sig_addr  = 0;
  sig_magic = false;
  raddr_b   = 0;      // Not set for branch

	alu.add.output_pack = V3D_QPU_PACK_NONE;
	alu.mul.output_pack = V3D_QPU_PACK_NONE;

  if (!instr_unpack(in_code, this)) {
    warn << "Instr:init: call to instr_unpack failed.";
    return;
  }


  if (is_branch()) {
    if (!branch.ub) {
      // take over the value anyway
      branch.bdu = (v3d_qpu_branch_dest) ((in_code >> 15) & 0b111);
    }
  }
}


/**
 * Convert conditions from Target source to v3d
 *
 * Incoming conditions are vc4 only, the conditions don't exist on v3d.
 * They therefore need to be translated.
 */
void Instr::set_branch_condition(V3DLib::BranchCond src_cond) {
  // TODO How to deal with:
  //
  //      instr.na0();
  //      instr.a0();

  if (src_cond.tag == BranchCond::COND_ALWAYS) {
     return;  // nothing to do
  } else if (src_cond.tag == BranchCond::COND_ALL) {
    switch (src_cond.flag) {
      case ZC:
      case NC:
        set_branch_condition(V3D_QPU_BRANCH_COND_ALLNA);
        break;
      case ZS:
      case NS:
        set_branch_condition(V3D_QPU_BRANCH_COND_ALLA);
        break;
      default:
        assertq(false, "Unknown branch condition under COND_ALL");
    }
  } else if (src_cond.tag == BranchCond::COND_ANY) {
    switch (src_cond.flag) {
      case ZC:
      case NC:
        set_branch_condition(V3D_QPU_BRANCH_COND_ANYNA);  // TODO: verify
        break;
      case ZS:
      case NS:
        set_branch_condition(V3D_QPU_BRANCH_COND_ANYA);   // TODO: verify
        break;
      default:
        debug_break("Unknown branch condition under COND_ANY");  // Warn me if this happens
    }
  } else {
    debug_break("Branch condition not COND_ALL or COND_ANY");  // Warn me if this happens
  }
}


///////////////////////////////////////////////////////////////////////////////
// End class Instr
///////////////////////////////////////////////////////////////////////////////

// from mesa/src/broadcom/qpu/qpu_pack.c
#define QPU_MASK(high, low) ((((uint64_t)1<<((high)-(low)+1))-1)<<(low))
#define QPU_GET_FIELD(word, field) ((uint32_t)(((word)  & field ## _MASK) >> field ## _SHIFT))

#define VC5_QPU_BRANCH_MSFIGN_SHIFT         21
#define VC5_QPU_BRANCH_MSFIGN_MASK          QPU_MASK(22, 21)

#define VC5_QPU_BRANCH_COND_SHIFT           32
#define VC5_QPU_BRANCH_COND_MASK            QPU_MASK(34, 32)
// End from mesa/src/broadcom/qpu/qpu_pack.c



bool Instr::compare_codes(uint64_t code1, uint64_t code2) {
  if (code1 == code2) {
    return true;
  }

  // Here's the issue:
  // For the branch instruction, if field branch.ub != true, then field branch bdu is not used
  // and can have any value.
  // So for a truthful compare, in this special case the field needs to be ignored.
  // Determine if this is a branch
  auto is_branch = [] (uint64_t code) -> bool {
    uint64_t mul_op_mask = ((uint64_t) 0xb11111) << 58;
    bool is_mul_op = 0 != ((code & mul_op_mask) >> 58);

    uint64_t branch_sig_mask = ((uint64_t) 1) << 57;
    bool has_branch_sig = 0 != (code & branch_sig_mask) >> 57;

    return (!is_mul_op && has_branch_sig);
  };


  if (!is_branch(code1) || !is_branch(code2)) {
    return false;  // Not the special case we're looking for
  }

  // Hypothesis: non-usage of bdu depends on these two fields

  uint32_t cond   = QPU_GET_FIELD(code1, VC5_QPU_BRANCH_COND);
  uint32_t msfign = QPU_GET_FIELD(code1, VC5_QPU_BRANCH_MSFIGN);

  if (cond == 0) {
    //instr->branch.cond = V3D_QPU_BRANCH_COND_ALWAYS;
  } else if (V3D_QPU_BRANCH_COND_A0 + (cond - 2) <= V3D_QPU_BRANCH_COND_ALLNA) {
    cond = V3D_QPU_BRANCH_COND_A0 + (cond - 2);
  }

  if (cond == 2 && msfign == V3D_QPU_MSFIGN_NONE) {
    // Zap out the bdu field and compare again
    uint64_t bdu_mask = ((uint64_t) 0b111) << 15;
    code1 = code1 & ~bdu_mask;
    code2 = code1 & ~bdu_mask;

    return code1 == code2;
  }

#ifdef DEBUG
  printf("compare_codes diff: %s\n", binaryValue(code1 ^ code2).c_str());
  printf("code1: %s\n", mnemonic(code1).c_str());
  printf("code2: %s\n", mnemonic(code2).c_str());

  breakpoint
#endif  // DEBUG

  return false;
}


///////////////////////////////////////////////////////////////////////////////
// Conditions branch instructions
///////////////////////////////////////////////////////////////////////////////

void Instr::set_branch_condition(v3d_qpu_branch_cond cond) {
  assert(is_branch());  // Branch instruction-specific
  branch.cond = cond;
}

///////////////////////////////////////////////////////////////////////////////
// End Conditions  branch instructions
///////////////////////////////////////////////////////////////////////////////

void Instr::set_sig_addr(Location const &loc) {
  sig_magic = !loc.is_rf();
  sig_addr = loc.to_waddr();
}


DestReg Instr::sig_dest() const {
  if (uses_sig_dst()) {
    return DestReg(sig_addr, sig_magic);
  }

  return DestReg();
}


DestReg Instr::add_dest() const {
  if (alu.add.op != V3D_QPU_A_NOP) {
		//breakpoint;
    return DestReg(alu.add.waddr, alu.add.magic_write);
  }

  return DestReg();
}


DestReg Instr::mul_dest() const {
  if (alu.mul.op != V3D_QPU_M_NOP) {
    return DestReg(alu.mul.waddr, alu.mul.magic_write);
  }

  return DestReg();
}


DestReg Instr::add_src_dest(v3d_qpu_input src) const {
  if (src.mux < V3D_QPU_MUX_A) {
    return DestReg(src.mux, true);
  } else if (src.mux == V3D_QPU_MUX_A) {
    return DestReg(raddr_a, false);
  } else if (!sig.small_imm_b) {
    return DestReg(raddr_b, false);
  }

  return DestReg();
}


DestReg Instr::add_src_a() const {
	if (Oper::num_operands(alu.add.op) == 0) return DestReg();
  if (alu.add.op == V3D_QPU_A_NOP) return DestReg();

  return add_src_dest(alu.add.a);
}


DestReg Instr::add_src_b() const {
	if (Oper::num_operands(alu.add.op) < 2) return DestReg();
  if (alu.add.op == V3D_QPU_A_NOP) return DestReg();

  return add_src_dest(alu.add.b);
}


DestReg Instr::mul_src_a() const {
  if (alu.mul.op == V3D_QPU_M_NOP) return DestReg();
  return add_src_dest(alu.mul.a);
}


DestReg Instr::mul_src_b() const {
  if (alu.mul.op == V3D_QPU_M_NOP) return DestReg();
  return add_src_dest(alu.mul.b);
}


bool Instr::is_src(DestReg const &dst_reg) const {
  if (is_branch()) return false;
  if (!dst_reg.used()) return false;

  return add_src_a() == dst_reg
      || add_src_b() == dst_reg
      || mul_src_a() == dst_reg
      || mul_src_b() == dst_reg;
}


bool Instr::is_dst(DestReg const &dst_reg) const {
  // Note that branch instruction can technically have a sig destination
  if (!dst_reg.used()) return false;

  return sig_dest() == dst_reg
      || add_dest() == dst_reg
      || mul_dest() == dst_reg;
}


void Instr::alu_add_dst(Location const &dst) {
  if (dst.is_rf()) {
    alu.add.magic_write = false; // selects address in register file
  } else {
    alu.add.magic_write = true;  // selects register

		assert(dst.is_reg());
  }

  alu.add.waddr = dst.to_waddr();
  alu.add.output_pack = dst.output_pack();
}


bool Instr::mux_in_use(CheckSrc check_src, v3d_qpu_mux mux) const {

	return
		(check_src != CHECK_ADD_A && m_alu_add_a_set && alu.add.a.mux == mux ) ||
		(check_src != CHECK_ADD_B && m_alu_add_b_set && alu.add.b.mux == mux ) ||
		(check_src != CHECK_MUL_A && m_alu_mul_a_set && alu.mul.a.mux == mux ) ||
		(check_src != CHECK_MUL_B && m_alu_mul_b_set && alu.mul.b.mux == mux );
}


bool Instr::raddr_a_is_safe(uint8_t raddr, CheckSrc check_src) const {
	if (Platform::compiling_for_vc7()) return true;  // Always safe for vc7

  if (!mux_in_use(check_src, V3D_QPU_MUX_A)) return true;
  return (raddr_a == raddr);
}


bool Instr::raddr_b_is_safe(uint8_t raddr, CheckSrc check_src) const {
	if (Platform::compiling_for_vc7()) return true;  // Always safe for vc7

  if (!mux_in_use(check_src, V3D_QPU_MUX_B)) return true;
  if (sig.small_imm_b) return false; 
  return (raddr_b == raddr);
}


/**
 * Following for vc6
 */
bool Instr::alu_set_src(Source const &src, v3d_qpu_input &input, CheckSrc check_src) {
	assert(!Platform::compiling_for_vc7());

	auto &mux = input.mux;

	if (src.is_small_imm()) {
		BaseSource tmp_src(src);
		set_mul_small_imm(*this, tmp_src, input, check_src);
		// warn << "alu_set_src small_imm";

		auto const &imm	= src.small_imm();

		if (sig.small_imm_b) {
			// warn << "alu_set_a: small imm same value";

			// If same value, all is well
			assertq(raddr_b == imm.val(), "alu_mul_b: small imm different value on vc6");
		  input.mux = V3D_QPU_MUX_B;
		} else {
			// Reserve the small imm
			// warn << "small imm different value or not present";
    	if (mux_in_use(check_src, V3D_QPU_MUX_B)) {
				assertq(false, "MUX_B in use for non-small_imm");
			}

			sig.small_imm_b = true;
			input.mux = V3D_QPU_MUX_B;
			raddr_b = imm.val();
			input.unpack = imm.input_unpack();
		}
  } else if (src.is_location()) {
		// warn << "alu_set_src location";

    Location const &loc = src.location();

    if (loc.is_reg()) {
      mux = loc.to_mux();
    } else if (raddr_a_is_safe(loc.to_waddr(), check_src)) {
      raddr_a = loc.to_waddr(); 
      mux = V3D_QPU_MUX_A;
    } else if (raddr_b_is_safe(loc.to_waddr(), check_src)) {
      raddr_b = loc.to_waddr(); 
      mux = V3D_QPU_MUX_B;
    } else {
			warning("alu_set_src: raddr_a and raddr_b both in use");
      return false;
    }

		input.unpack = loc.input_unpack();
	}

  return true;
}


bool Instr::alu_add_set(Location const &dst, Source const &in_a, Source const &in_b) {
  alu_add_dst(dst);

	BaseSource a(in_a);
	BaseSource b(in_b);

	// TODO: Likely need this on alu_mul_set as well
	//       Better would be to test alu/mul together
	if (Platform::compiling_for_vc7()) {
		// TODO: I *think* this is correct, but need to research further
		if (a.is_small_imm() && b.is_small_imm()) {
				cerr << "shl(): can not pass in two immediates: " 
					   << a.dump() << ", " << b.dump() << "\n" << thrw;
		}
	} else {
		if (a.is_small_imm() && b.is_small_imm()) {
			if (a != b) {
				cerr << "shl(): can not pass in two different immediates: " 
					   << a.dump() << " != " << b.dump() << "\n" << thrw;
			}
		}
	}

  bool ret = alu_add_a(a) && alu_add_b(b);

	if (!ret) {
		breakpoint;  // Warn me when this happens
	}

	return ret;
}


/**
 * @param overwrite If true, overwrite of existing value is intentional
 */
bool Instr::alu_add_a(BaseSource const &src, bool overwrite) {
	assert(src.is_set());
	assert(!m_external_init);
	if (m_alu_add_a_set && !overwrite) {
		warn << "alu_add_a overwriting existing src value";
	}

	if (src.is_small_imm()) {
		if (Platform::compiling_for_vc7()) {
			sig.small_imm_a = true;
			alu.add.a.raddr = src.val();
		} else {
			set_mul_small_imm(*this, src, alu.add.a, CHECK_ADD_A);
		}

	} else if (!alu_set_src(src, alu.add.a, CHECK_ADD_A)) {
		return false;
	}

  alu.add.a.unpack = src.unpack();
	m_alu_add_a_set  = true;
	return true;
}


/**
 * @param overwrite If true, overwrite of existing value is intentional
 */
bool Instr::alu_add_b(BaseSource const &src, bool overwrite) {
	assert(src.is_set());
	assert(!m_external_init);
	if (m_alu_add_b_set && !overwrite) {
		warn << "alu_add_b overwriting existing src value";
	}

	if (src.is_small_imm()) {
		if (Platform::compiling_for_vc7()) {
			sig.small_imm_b = true;
			alu.add.b.raddr = src.val();
		} else {
			set_mul_small_imm(*this, src, alu.add.b, CHECK_ADD_B);
		}

	} else if (!alu_set_src(src, alu.add.b, CHECK_ADD_B)) {
		return false;
	}

  alu.add.b.unpack = src.unpack();
	m_alu_add_b_set  = true;
	return true;
}


void Instr::alu_mul_dst(Location const &dst) {
  if (dst.is_rf()) {
    alu.mul.magic_write = false; // selects address in register file
  } else {
    alu.mul.magic_write = true;  // selects register

		assert(dst.is_reg());
		//Log::warn << "alu_add_dst acc detected";
  }

  alu.mul.waddr = dst.to_waddr();
  alu.mul.output_pack = dst.output_pack();
}


/**
 * Insight
 * -------
 *
 * vc6 mux:
 * 	V3D_QPU_MUX_A: use raddr_a
 * 	V3D_QPU_MUX_B: use raddr_b
 *
 * This gives a tiny bit of extra flexibilty, because it's possible
 * to select raddr_a/b for add/mul.a/b.
 * TODO: Examine this (some time, ever, not up to it now).
 *
 * For the time being, we stick to V3D_QPU_MUX_A for add/mul.a and V3D_QPU_MUX_B for add/mul.b.
 *
 * This is different from vc4, where the values indicate using rfA or rfB.
 */
bool Instr::alu_mul_a(BaseSource const &src, bool overwrite) {
	assert(src.is_set());
	assert(!m_external_init);
	if (m_alu_mul_a_set && !overwrite) {
		warn << "alu_mul_a overwriting existing src value";
	}

	if (src.is_small_imm()) {
		if (Platform::compiling_for_vc7()) {
			sig.small_imm_c = true;
			alu.mul.a.raddr = src.val();
		} else {
			set_mul_small_imm(*this, src, alu.mul.a, CHECK_MUL_A);
		}

	} else if (!alu_set_src(src, alu.mul.a, CHECK_MUL_A)) {
		return false;
	}

  alu.mul.a.unpack = src.unpack();
	m_alu_mul_a_set  = true;
	return true;
}


/**
 * Check if the given source can be inserted into mul b.
 *
 * This assumes that add b has been filled in previously.
 */
bool Instr::check_safe(BaseSource const &src, CheckSrc check_src) const {
	assert(src.is_set());

	if (src.is_small_imm()) {
		if (!Platform::compiling_for_vc7()) {
			return check_small_imm(*this, src);
		}
	} else if (src.is_reg()) {
		if (Platform::compiling_for_vc7()) {
			warning("check_safe: can not use registers on vc7");
			return false;
		}
	} else {
		return raddr_a_is_safe(src.val(), CHECK_MUL_B)
		    || raddr_b_is_safe(src.val(), CHECK_MUL_B);
	}

	return true;
}


bool Instr::alu_set_src(BaseSource const &src, v3d_qpu_input &input, CheckSrc check_src) {
	if (src.is_reg()) {
		assertq(!Platform::compiling_for_vc7(), "alu_set_src: can not use registers on vc7");
		// src.val() is a mux value
		input.mux = (v3d_qpu_mux) src.val();
	} else {
		if (Platform::compiling_for_vc7()) {
			input.raddr = src.val();
		} else {
			if (raddr_a_is_safe(src.val(), check_src)) {
				input.mux = V3D_QPU_MUX_A;
				raddr_a = src.val();
			}	else if (raddr_b_is_safe(src.val(), check_src)) {
				input.mux = V3D_QPU_MUX_B;
				raddr_b = src.val();
			} else {
				/*
				   Did extensive research, the test was valid for 100% of the cases.

				cerr << "alu_set_src: rf fails raddr_a/b check:\n"
					   << "Src  : " << src.dump() << ", can not assign to " << dump_check(check_src) << "\n"
					   << "Instr: " << mnemonic() << "\n";
				*/
				return false;
			}
		}
	}

	return true;
}	


bool Instr::alu_mul_b(BaseSource const &src, bool overwrite) {
	assert(src.is_set());
	assert(!m_external_init);
	if (m_alu_mul_b_set && !overwrite) {
		warn << "alu_mul_b overwriting existing src value";
	}

	if (src.is_small_imm()) {
		if (Platform::compiling_for_vc7()) {
			sig.small_imm_d = true;
			alu.mul.b.raddr = src.val();
		} else {
			set_mul_small_imm(*this, src, alu.mul.b, CHECK_MUL_B);
		}

	} else if (!alu_set_src(src, alu.mul.b, CHECK_MUL_B)) {
		return false;
	}
		
  alu.mul.b.unpack = src.unpack();
	m_alu_mul_b_set  = true;
	return true;
}

/**
 * TODO consolidate override with b param
 */
bool Instr::alu_mul_set(Location const &dst, Source const &a) {
	alu_mul_dst(dst);

	bool ret = true;

	if (Platform::compiling_for_vc7()) {
		ret = alu_mul_a(a);
		assert(ret);
	} else {
  	ret = alu_set_src(a, alu.mul.a, CHECK_MUL_A);
	}

	if (ret) {
  	alu.mul.a.unpack = a.input_unpack();
	}

	return ret;
}


/**
 * Apparently you *can* pass small imm's.
 * if alu.add.b is available, it can be used for mul.
 */
bool Instr::alu_mul_set(Location const &dst, Source const &in_a, Source const &in_b) { 
	alu_mul_dst(dst);

	BaseSource a(in_a);
	BaseSource b(in_b);

	if (a.is_small_imm() && b.is_small_imm()) {
		if (a.val() != b.val()) {
    	cerr << "alu_mul_set: can not pass two different small immediates." << thrw;
		}
	}

  alu_mul_a(a);
 	alu_mul_b(b);

	return true;  // Dummy return value. TODO cleanup
}


/**
 * 
 */
bool Instr::alu_add_set(Target::Instr const &src_instr) {
  assert(add_nop());
  assert(mul_nop());

  auto const &src_alu = src_instr.ALU;
  auto dst = encodeDestReg(src_instr);
  assert(dst);

  auto reg_a = src_alu.srcA;
  auto reg_b = src_alu.srcB;

  v3d_qpu_add_op add_op;
  if (OpItems::get_add_op(src_alu, add_op, false)) {
    alu.add.op = add_op;
    return alu_add_set(*dst, reg_a, reg_b);
  }

  return false;
}


/**
 * @return true if mul instruction set, false otherwise
 */
bool Instr::alu_mul_set(Target::Instr const &src_instr) {
  assert(mul_nop());
  auto const &alu = src_instr.ALU;
  auto dst = encodeDestReg(src_instr);
  assert(dst);

  v3d_qpu_mul_op mul_op;

  //
  // Get the v3d mul equivalent of a target lang add alu instruction.
  //

  // Special case: OR with same inputs can be considered a MOV
  // Handles rf-registers and accumulators only, fails otherwise
  // (ie. case combined tmua tmud won't work).
  if (alu.op == Enum::A_BOR) {
    bool same_sources = (src_instr.dest().tag <= ACC)
                     && (alu.srcA == alu.srcB)
                     && (alu.srcA.is_imm() || alu.srcA.reg().tag <= ACC);  // Verified: this check is required, won't work with special registers

    if (same_sources) {
      mul_op = V3D_QPU_M_MOV;  // TODO consider _FMOV as well
    } else {
      return false;  // Can't convert
    }
  }

  if (!V3DLib::v3d::instr::OpItems::get_mul_op(alu, mul_op)) {
		warning("alu_mul_set(): Can't convert mul op");
    return false;  // Can't convert
  }

  auto reg_a = alu.srcA;
  auto reg_b = alu.srcB;

  this->alu.mul.op = mul_op;

  if (alu_mul_set(*dst, reg_a, reg_b)) {
    flags.mc = translate_assign_cond(src_instr.assign_cond());


    // TODO shouldn't push tag be done as well? Check
    // Normally set with set_push_tag()
    //this->alu.mul.m_setCond = alu.m_setCond;

    //std::cout << "alu_mul_set(ALU) result: " << mnemonic(true) << std::endl;
    return true;
  }

  return false;
}


bool Instr::alu_set(Target::Instr const &src_instr) {
    return alu_add_set(src_instr) || alu_mul_set(src_instr);
}


///////////////////////////////////////////////////////////////////////////////
// Label support
//////////////////////////////////////////////////////////////////////////////

void Instr::label(int val) {
  assert(val >= 0);
  m_label = val;
}


int Instr::branch_label() const {
  assert(is_branch_label());
  return m_label;
}


bool Instr::is_branch_label() const {
  return type == V3D_QPU_INSTR_TYPE_BRANCH && m_label >= 0;
}


void Instr::label_to_target(int offset) {
  assert(!m_is_label);
  assert(0 <= branch_label());  // only do this to a branch instruction with a label
  assert(branch.offset == 0);   // Shouldn't have been set already

  // branch needs 4 delay slots before executing, hence the 4
  // This means that 3 more instructions will execute after the loop before jumping
  branch.offset = (unsigned) 8*(offset - 4);

  m_label = -1;
}


/*
 * Note that packing fields are not set here.
 * If you require them, it must be done elsewhere
 */
std::unique_ptr<Location> Instr::add_alu_dst() const {
  std::unique_ptr<Location> res;

  if (alu.add.magic_write) {
    // accumulator
    res.reset(new Register("", (v3d_qpu_waddr) alu.add.waddr, (v3d_qpu_mux) alu.add.waddr, true));
  } else {
    // rf-register
    res.reset(new RFAddress(alu.add.waddr));
  }

  assert(res);
  return res;
}


std::unique_ptr<Location> Instr::mul_alu_dst() const {
  std::unique_ptr<Location> res;

  if (alu.mul.magic_write) {
    // accumulator
    res.reset(new Register("", (v3d_qpu_waddr) alu.mul.waddr, (v3d_qpu_mux) alu.mul.waddr, true));
  } else {
    // rf-register
    res.reset(new RFAddress(alu.mul.waddr));
  }

  assert(res);  return res;
}


/**
 * Return the contents of alu add a
 */
BaseSource Instr::alu_add_dst() const {
  BaseSource res;
	if (add_nop()) return res;

	res.set_from_dst(alu.add.waddr, alu.add.magic_write);
	return res;
}


BaseSource Instr::alu_mul_dst() const {
  BaseSource res;
	if (mul_nop()) return res;

	res.set_from_dst(alu.mul.waddr, alu.mul.magic_write);
	return res;
}


BaseSource Instr::sig_dst() const {
  BaseSource res;

  if (!uses_sig_dst()) return res;

  res.set_from_dst(sig_addr, sig_magic);
	return res;
}


/**
 * Return the contents of alu mul a
 *
 * Interesting thought:
 * --------------------
 *
 * * vc6: there are only two raddr fields per instruction for both add and mul.
 *   There are limited ways to combine them:
 *      - add a is mux and then mul a is free
 *      - mul a is mux add then add a is free
 *      - add a is rf and then mul a must be mux OR same as a
 *      - mul b is rf and then mul a must be mux OR same as b
 *
 *   TODO: Perhaps this needs some further rethinking; map out all possible combinations.
 *
 *   small_imm is a special case here since is only applies to add b
 *
 *
 */
BaseSource Instr::alu_add_a() const { return BaseSource(*this, CHECK_ADD_A); }
BaseSource Instr::alu_add_b() const { return BaseSource(*this, CHECK_ADD_B); }
BaseSource Instr::alu_mul_a() const { return BaseSource(*this, CHECK_MUL_A); }
BaseSource Instr::alu_mul_b() const { return BaseSource(*this, CHECK_MUL_B); }



}  // namespace instr


///////////////////////////////////////////////////////////////////////////////
// Class Instructions
//////////////////////////////////////////////////////////////////////////////

Instructions &Instructions::comment(std::string msg, bool to_front) {
  assert(!empty());

  if (to_front) {
    front().comment(msg);
  } else {
    back().comment(msg);
  }

  return *this;
}


/**
 * Use param as run condition for current instruction(s)
 */
void Instructions::set_cond_tag(AssignCond cond) {
  for (auto &instr : *this) {
    instr.set_cond_tag(cond);
  }
}


bool Instructions::check_consistent() const {
  bool ret = true;

  for (auto &instr : *this) {
    if (!instr.check_dst()) {
      std::string err;
      err << "Overlapping dst registers in instruction:" << instr.mnemonic();  // TODO: output index if required
      ret = false;
    }
  }

  return ret;
}


ByteCode Instructions::bytecode() const {
  ByteCode ret;
  for (auto const &instr : *this) {
    ret << instr.bytecode(); 
  }

	return ret;
}


std::string Instructions::dump() const {
	std::string ret;

	int count = 0;
  for (auto const &instr : *this) {
    ret << count << "; " << instr.mnemonic(true) << "\n"; 
		count++;
  }

	return ret;
}

}  // namespace v3d
}  // namespace V3DLib
