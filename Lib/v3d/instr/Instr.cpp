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

uint64_t Instr::NOP() {
  if (devinfo_ver() == 42) {
		return 0x3c003186bb800000;  // This is actually 'nop nop'
	} else {
		// 7x, vc7
		return 0x38000000bb03f000;
	}
}


Instr::Instr(uint64_t in_code) {
  init(in_code);
}


bool Instr::is_branch() const {
  return (type == V3D_QPU_INSTR_TYPE_BRANCH);
}


/**
 * Return true if any of the small_imm flags are set (there are four).
 */
bool Instr::has_small_imm() const {
#if USE_MESA == 1
	return sig.small_imm;
#else
	return sig.small_imm_a
	    || sig.small_imm_b
	    || sig.small_imm_c
	    || sig.small_imm_d;
#endif
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


bool Instr::flag_set() const {
  return (flags.ac || flags.mc || flags.apf || flags.mpf || flags.auf || flags.muf);
}


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
    error("More than one ld-signal with dst register set");
    return false;
  }

  bool ret = true;

  DestReg sig_dst = sig_dest();
  DestReg add_dst = add_dest();
  DestReg mul_dst = mul_dest();

  if (sig_dst == add_dst) {
    breakpoint
    error("signal dst register same as add alu dst register");
    ret = false;
  }

  if (sig_dst == mul_dst) {
    breakpoint
    error("signal dst register same as mul alu dst register");
    ret = false;
  }

  if (add_dst == mul_dst) {
    breakpoint
    error("add alu dst register same as mul alu dst register");
    ret = false;
  }

  return ret;
}


std::string Instr::dump() const {
  char buffer[10*1024];
  instr_dump(buffer, const_cast<Instr *>(this));
  return std::string(buffer);
}


std::string Instr::pretty_instr() const {

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

#if USE_MESA == 1
      // Only two possibilities here: r5 or small imm (sig for small imm not set!)
      if (alu.mul.b == V3D_QPU_MUX_R5) {
        ret << ", r5";
      } else if (alu.mul.b == V3D_QPU_MUX_B) {
        ret << ", " << raddr_b;
      } else {
        assertq(false, "pretty_instr(): unexpected mux value for mul b for rotate", true);
      }
#else
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
	        assertq(false, "pretty_instr(): unexpected mux value for mul b for rotate", true);
	      }
			}
#endif

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

  std::string out = pretty_instr();
  ret << out;

  if (with_comments && !InstructionComment::comment().empty()) {
    ret << emit_comment((int) out.size());
  }

  return ret;
}


uint64_t Instr::code() const {
  uint64_t repack = instr_pack(const_cast<Instr *>(this));
  return repack;
}


std::string Instr::dump(uint64_t in_code) {
  Instr instr(in_code);
  return instr.dump();
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
  sig_addr = 0;
  sig_magic = false;
  raddr_b = 0; // Not set for branch

  if (!instr_unpack(in_code, this)) {
    warning("Instr:init: call to instr_unpack failed.");
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

// <<<<<<<<<<<<<<<<<<<<<<<<
  if (src_cond.tag == BranchCond::COND_ALWAYS) {
     return;  // nothing to do
  } else if (src_cond.tag == BranchCond::COND_ALL) {
/*   ======================== * /
  if (src_cond.tag == COND_ALWAYS) {
    return;  // nothing to do
  } else if (src_cond.tag == COND_ALL) {
/ * >>>>>>>>>>>>>>>>>>>>>>>> */
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
// <<<<<<<<<<<<<<<<<<<<<<<<
        assertq(false, "Unknown branch condition under COND_ALL");
    }
  } else if (src_cond.tag == BranchCond::COND_ANY) {
/*   ======================== * /
        debug_break("Unknown branch condition under COND_ALL");  // Warn me if this happens
    }
  } else if (src_cond.tag == COND_ANY) {
/ * >>>>>>>>>>>>>>>>>>>>>>>> */
    switch (src_cond.flag) {
      case ZC:
      case NC:
/* <<<<<<<<<<<<<<<<<<<<<<<<
        assertq(false, "Unknown branch condition under COND_ANY");
   ======================== */
        set_branch_condition(V3D_QPU_BRANCH_COND_ANYNA);  // TODO: verify
/* >>>>>>>>>>>>>>>>>>>>>>>> */
        break;
      case ZS:
      case NS:
        set_branch_condition(V3D_QPU_BRANCH_COND_ANYA);   // TODO: verify
        break;
      default:
/* <<<<<<<<<<<<<<<<<<<<<<<<
        assertq(false, "Unknown branch condition under COND_ANY");
   ======================== */
        debug_break("Unknown branch condition under COND_ANY");  // Warn me if this happens
/* >>>>>>>>>>>>>>>>>>>>>>>> */
    }
  } else {
/* <<<<<<<<<<<<<<<<<<<<<<<<
    assertq(false, "Branch condition not COND_ALL or COND_ANY");
   ======================== */
    debug_break("Branch condition not COND_ALL or COND_ANY");  // Warn me if this happens
/* >>>>>>>>>>>>>>>>>>>>>>>> */
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
  printf("%s\n", dump(code1).c_str());
  printf("code2: %s\n", mnemonic(code2).c_str());
  printf("%s\n", dump(code2).c_str());

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

DestReg Instr::sig_dest() const {
  if (uses_sig_dst()) {
    return DestReg(sig_addr, sig_magic);
  }

  return DestReg();
}


DestReg Instr::add_dest() const {
  if (alu.add.op != V3D_QPU_A_NOP) {
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


#if USE_MESA == 1
DestReg Instr::add_src_dest(v3d_qpu_mux src) const {
#else
DestReg Instr::add_src_dest(v3d_qpu_input src) const {
#endif
#if USE_MESA == 1
  if (src < V3D_QPU_MUX_A) {
    return DestReg(src, true);
  } else if (src == V3D_QPU_MUX_A) {
    return DestReg(raddr_a, false);
  } else if (!sig.small_imm) {  // must be MUX_B
#else
  if (src.mux < V3D_QPU_MUX_A) {
    return DestReg(src.mux, true);
  } else if (src.mux == V3D_QPU_MUX_A) {
    return DestReg(raddr_a, false);
  } else if (!sig.small_imm_b) {
#endif
    return DestReg(raddr_b, false);
  }

  return DestReg();
}


DestReg Instr::add_src_a() const {
  if (alu.add.op == V3D_QPU_A_NOP) return DestReg();
  return add_src_dest(alu.add.a);
}


DestReg Instr::add_src_b() const {
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


void Instr::alu_add_set_dst(Location const &dst) {
  if (dst.is_rf()) {
    alu.add.magic_write = false; // selects address in register file
  } else {
    alu.add.magic_write = true;  // selects register
  }

  alu.add.waddr = dst.to_waddr();
  alu.add.output_pack = dst.output_pack();
}


bool Instr::raddr_a_is_safe(Location const &loc, CheckSrc check_src) const {
  assert(loc.is_rf());
  if (!raddr_in_use(check_src, V3D_QPU_MUX_A)) return true;
  return (raddr_a == loc.to_waddr());
}


bool Instr::raddr_in_use(CheckSrc check_src, v3d_qpu_mux mux) const {
  bool in_use = false;

  switch (check_src) {  // Note reverse order, fall-thru intentional
#if USE_MESA == 1
    case CHECK_MUL_B:
      in_use = in_use || (alu.mul.a == mux);
    case CHECK_MUL_A:
      in_use = in_use || (alu.add.b == mux);
    case CHECK_ADD_B:
      in_use = in_use || (alu.add.a == mux);
#else
    case CHECK_MUL_B:
      in_use = in_use || (alu.mul.a.mux == mux);
    case CHECK_MUL_A:
      in_use = in_use || (alu.add.b.mux == mux);
    case CHECK_ADD_B:
      in_use = in_use || (alu.add.a.mux == mux);
#endif
    case CHECK_ADD_A:
      break;
  }

  return in_use;
}


bool Instr::raddr_b_is_safe(Location const &loc, CheckSrc check_src) const {
  assert(loc.is_rf());
  if (!raddr_in_use(check_src, V3D_QPU_MUX_B)) return true;
#if USE_MESA == 1
  if (sig.small_imm) return false; 
#else
  if (sig.small_imm_b) return false; 
#endif
  return (raddr_b == loc.to_waddr());
}


/**
 * Following for vc4 and vc6
 */
#if USE_MESA == 1
bool Instr::alu_set_src(Source const &src, v3d_qpu_mux &mux, CheckSrc check_src) {
	fatal("alu_set_src(): Not supporting mesa1 any more");
	return false;
}

#else

bool Instr::alu_set_src(Source const &src, v3d_qpu_input &input, CheckSrc check_src) {
	assert(!Platform::compiling_for_vc7());

	// vc6
	auto &mux = input.mux;

  if (src.is_location()) {
    Location const &loc = src.location();

    if (loc.is_acc()) {
      mux = loc.to_mux();
    } else if (raddr_a_is_safe(loc, check_src)) {
      raddr_a = loc.to_waddr(); 
      mux = V3D_QPU_MUX_A;
    } else if (raddr_b_is_safe(loc, check_src)) {
      raddr_b = loc.to_waddr(); 
      mux = V3D_QPU_MUX_B;
    } else {
			//warning("alu_set_src: raddr_a and raddr_b both in use");
      return false;
    }

  } else {
    // Handle small imm
    auto imm = src.small_imm();

    if (raddr_in_use(check_src, V3D_QPU_MUX_B)) {
      if (raddr_b != imm.to_raddr()) {
				//warning("alu_set_src: imm not equal to raddr_b");
				return false;
			}
	
			// Test not necessary because small_imm_b is set below anyway
			//assertq(sig.small_imm_b, "small_imm_b not set, expected.");
    }

    // All is well
		sig.small_imm_b  = true;
    raddr_b          = imm.to_raddr(); 
    mux              = V3D_QPU_MUX_B;
	}

  return true;
}

#endif


bool Instr::alu_add_set_a(Source const &src) {
	if (!Platform::compiling_for_vc7()) {
  	return alu_set_src(src, alu.add.a, CHECK_ADD_A);
	}

 	if (src.is_location()) {
   	Location const &loc = src.location();
     alu.add.a.raddr = loc.to_waddr(); 
	} else {
		// Small Imm on a for vc7
    auto imm = src.small_imm();
    alu.add.a.raddr = imm.to_raddr(); 
		sig.small_imm_a = 1;
	}

  alu.add.a.unpack = src.input_unpack();

  return true;
}


bool Instr::alu_mul_set_a(Source const &src) {
	if (!Platform::compiling_for_vc7()) {
  	return alu_set_src(src, alu.mul.a, CHECK_MUL_A);
	}

 	if (src.is_location()) {
   	Location const &loc = src.location();
    alu.mul.a.raddr = loc.to_waddr(); 
	} else {
		// Small Imm on a for vc7
    auto imm = src.small_imm();
    alu.mul.a.raddr = imm.to_raddr(); 
		sig.small_imm_c = 1;
	}

  alu.mul.a.unpack = src.input_unpack();

  return true; 
}


bool Instr::alu_add_set_b(Source const &src) {
	if (!Platform::compiling_for_vc7()) {
  	return alu_set_src(src, alu.add.b, CHECK_ADD_B);
	}

 	if (src.is_location()) {
   	Location const &loc = src.location();
    alu.add.b.raddr = loc.to_waddr(); 
	} else {
		// Small Imm on b for vc7
    auto imm = src.small_imm();
    alu.add.b.raddr = imm.to_raddr(); 
		sig.small_imm_b = 1;
	}

  alu.add.b.unpack = src.input_unpack();

  return true;
}


bool Instr::alu_mul_set_b(Source const &src) {
	if (!Platform::compiling_for_vc7()) {
  	return alu_set_src(src, alu.mul.b, CHECK_MUL_B);
	}

 	if (src.is_location()) {
   	Location const &loc = src.location();
    alu.mul.b.raddr = loc.to_waddr(); 
	} else {
		// Small Imm on b for vc7
    auto imm = src.small_imm();
    alu.mul.b.raddr = imm.to_raddr(); 
		sig.small_imm_d = 1;
	}

  alu.mul.b.unpack = src.input_unpack();

  return true;
}


bool Instr::alu_add_set(Location const &dst, Source const &a, Source const &b) {
  alu_add_set_dst(dst);

	bool ret = true;

  ret = alu_add_set_a(a);
		
	if (ret) {
 		ret = alu_add_set_b(b);
	}

	if (!ret) {
    throw Exception("alu_add_set failed");
	}

  return ret;
}


void Instr::alu_mul_dst(Location const &dst) {
  if (dst.is_rf()) {
    alu.mul.magic_write = false; // selects address in register file
  } else {
    alu.mul.magic_write = true;  // selects register
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
void Instr::alu_mul_a(BaseSource const &src) {
	assert(src.is_set());

	if (src.is_small_imm()) {
		assertq(Platform::compiling_for_vc7(), "alu_mul_a: can not use small imm on vc6");
		sig.small_imm_c = true;
		alu.mul.a.raddr = src.val();
	} else if (src.is_reg()) {
		assertq(!Platform::compiling_for_vc7(), "alu_mul_a: can not use registers on vc7");
		// src.val() is a mux value
		alu.mul.a.mux = (v3d_qpu_mux) src.val();
	} else {
		// rf location
		if (Platform::compiling_for_vc7()) {
			alu.mul.a.raddr = src.val();
		} else {
			// TODO : there can be a conflict here with raddr_a for add
			alu.mul.a.mux = V3D_QPU_MUX_A;
			raddr_a = src.val();
		}
	}
}


/**
 * Check if the given source can be inserted into mul a.
 *
 * This assumes that add a has been filled in previously.
 */
bool Instr::alu_mul_a_safe(BaseSource const &src) {
	assert(src.is_set());

	if (src.is_small_imm()) {
		if (!Platform::compiling_for_vc7()) {
			//warning("alu_mul_a_safe: can not use small imm on vc6");
			return false;
		}
	} else if (src.is_reg()) {
		if (Platform::compiling_for_vc7()) {
			//warning("alu_mul_a_safe: can not use registers on vc7");
			return false;
		}
	} else {
		// rf location - always possible for vc7
		if (!Platform::compiling_for_vc7()) {
			if (alu.add.a.mux == V3D_QPU_MUX_A) {
				if (raddr_a == src.val()) {
					return true;   // Can reuse raddr_a for mul.a
				}
			} else if (alu.add.a.mux == V3D_QPU_MUX_B) { 
				if (raddr_b == src.val()) {
					return true;  // Can reuse raddr_b for mul.a
				}
			}

			//warning("alu_mul_a_safe: vc6 add raddr_a/b do not match with mul a");
			return false;
		}
	}

	return true;
}


/**
 * Check if the given source can be inserted into mul b.
 *
 * This assumes that add b has been filled in previously.
 */
bool Instr::alu_mul_b_safe(BaseSource const &src) {
	assert(src.is_set());

	if (src.is_small_imm()) {
		if (!Platform::compiling_for_vc7()) {
			//warning("alu_mul_b_safe: can not use small imm on vc6");
			return false;
		}
	} else if (src.is_reg()) {
		if (Platform::compiling_for_vc7()) {
			warning("alu_mul_b_safe: can not use registers on vc7");
			return false;
		}
	} else {
		// rf location - always possible for vc7
		if (!Platform::compiling_for_vc7()) {
			if (alu.add.b.mux == V3D_QPU_MUX_A) {
				if (raddr_a == src.val()) {
					return true;  // Can reuse raddr_a for mul.b
				}
			} else if (alu.add.b.mux == V3D_QPU_MUX_B) { 
				if (raddr_b == src.val()) {
					return true;  // Can reuse raddr_b for mul.b
				}
			}

			//warning("alu_mul_b_safe: vc6 add raddr_a/b do not match with mul b");
			return false;
		}
	}

	return true;
}


void Instr::alu_mul_b(BaseSource const &src) {
	assert(src.is_set());

	if (src.is_small_imm()) {
		assertq(Platform::compiling_for_vc7(), "alu_mul_a: can not use small imm on vc6");
		sig.small_imm_d = true;
		alu.mul.b.raddr = src.val();
	} else if (src.is_reg()) {
		assertq(!Platform::compiling_for_vc7(), "alu_mul_a: can not use registers on vc7");
		// src.val() is a mux value
		alu.mul.b.mux = (v3d_qpu_mux) src.val();
	} else {
		// rf location
		if (Platform::compiling_for_vc7()) {
			alu.mul.b.raddr = src.val();
		} else {
			alu.mul.b.mux = V3D_QPU_MUX_B;
			raddr_b = src.val();
		}
	}
}


bool Instr::alu_mul_set(Location const &dst, Source const &a, Source const &b) {
	alu_mul_dst(dst);

	bool ret;

	if (Platform::compiling_for_vc7()) {
		ret = alu_mul_set_a(a);
		assert(ret);
	
		ret = alu_mul_set_b(b);
		assert(ret);
	} else {
  	ret = alu_set_src(a, alu.mul.a, CHECK_MUL_A) && alu_set_src(b, alu.mul.b, CHECK_MUL_B);
	}

	if (ret) {
#if USE_MESA == 1
  	alu.mul.a_unpack = a.input_unpack();
  	alu.mul.b_unpack = b.input_unpack();
#else
  	alu.mul.a.unpack = a.input_unpack();
  	alu.mul.b.unpack = b.input_unpack();
#endif
	}

	return ret;
}


/**
 * 
 */
bool Instr::alu_add_set(V3DLib::Instr const &src_instr) {
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
bool Instr::alu_mul_set(V3DLib::Instr const &src_instr) {
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
  if (alu.op == ALUOp::A_BOR) {
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


bool Instr::alu_set(V3DLib::Instr const &src_instr) {
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


std::unique_ptr<Source> Instr::add_alu_a() const {
	auto const &src = alu.add.a;

#if USE_MESA == 1
	return alu_src(src);
#else
  std::unique_ptr<Source> res;

	if (Platform::compiling_for_vc7()) {
	  if (sig.small_imm_a) { // add a, small imm
	    res.reset(new Source(SmallImm((int) src.raddr, false)));
	  } else {               // add a, rf
	    res.reset(new Source(RFAddress(src.raddr)));
	  }
	} else {
		// vc6 - no small imm on a
		if (alu.add.a.mux == V3D_QPU_MUX_A) {
	    res.reset(new Source(RFAddress(raddr_a)));
		} else {
	    // Doesn't work: 
			// res.reset(new Source(Register(raddr_a)));
	    res.reset(new Source(RFAddress(raddr_a)));  // Use this for the time being
		}
	}

  assert(res);
  return res;
#endif
}


/**
 * Return the contents of alu add a
 */
BaseSource Instr::alu_add_a() const {
#if USE_MESA == 1
	fatal("alu_add_a(): does not support mesa1");
#endif

  BaseSource res;

	if (add_nop()) return res;

	if (Platform::compiling_for_vc7()) {
		// vc7 - no acc's
	  res.set(alu.add.a.raddr, sig.small_imm_a, false, false);
	} else {
		// vc6 - no small imm on a
		if (alu.add.a.mux == V3D_QPU_MUX_A) {
	    res.set(raddr_a, false, false, true );
		} else if (alu.add.a.mux == V3D_QPU_MUX_B) {
	    res.set(raddr_b, false, false, false );
		} else {
	    res.set(alu.add.a.mux, false, true, false);
		}
	}

	return res;
}


/**
 * Return the contents of alu add b
 */
BaseSource Instr::alu_add_b() const {
#if USE_MESA == 1
	fatal("alu_add_b(): does not support mesa1");
#endif

  BaseSource res;

	if (add_nop()) return res;

	if (Platform::compiling_for_vc7()) {
		// vc7 - no acc's
	  res.set(alu.add.b.raddr, sig.small_imm_b, false, false);
	} else {
		// vc6
		if (sig.small_imm_b) {
	    res.set(raddr_b, true, false, false );
		} else if (alu.add.b.mux == V3D_QPU_MUX_A) {
	    res.set(raddr_a, false, false, true );
		} else if (alu.add.b.mux == V3D_QPU_MUX_B) {
	    res.set(raddr_b, false, false, false );
		} else {
	    res.set(alu.add.b.mux, false, true, false);
		}
	}

	return res;
}


/**
 * Return the contents of alu mul a
 *
 * =================================================
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
BaseSource Instr::alu_mul_a() const {
#if USE_MESA == 1
	fatal("alu_mul_a(): does not support mesa1");
#endif

  BaseSource res;

	if (mul_nop()) return res;

	if (Platform::compiling_for_vc7()) {
		// vc7 - no acc's
	  res.set(alu.mul.a.raddr, sig.small_imm_c, false, false);
	} else {
		// vc6 - no small imm on imul a
		if (alu.mul.a.mux == V3D_QPU_MUX_A) {
	    res.set(raddr_a, false, false, true );
		} else if (alu.mul.a.mux == V3D_QPU_MUX_B) {
	    res.set(raddr_b, false, false, false );
		} else {
	    res.set(alu.mul.b.mux, false, true, false);
		}
	}

	return res;
}


/**
 * Return the contents of alu mul b
 */
BaseSource Instr::alu_mul_b() const {
#if USE_MESA == 1
	fatal("alu_mul_b(): does not support mesa1");
#endif

  BaseSource res;

	if (mul_nop()) return res;

	if (Platform::compiling_for_vc7()) {
		// vc7 - no acc's
	  res.set(alu.mul.b.raddr, sig.small_imm_d, false, false);
	} else {
		// vc6 - no small imm on mul b 
		if (alu.mul.b.mux == V3D_QPU_MUX_A) {
	    res.set(raddr_a, false, false, true );
		} else if (alu.mul.b.mux == V3D_QPU_MUX_B) {
	    res.set(raddr_b, false, false, false );
		} else {
	    res.set(alu.mul.b.mux, false, true, false);
		}
	}

	return res;
}


std::unique_ptr<Source> Instr::add_alu_b() const {
	auto const &src = alu.add.b;

#if USE_MESA == 1
	return alu_src(src);
#else
  std::unique_ptr<Source> res;

  if (sig.small_imm_b) { // add b, small imm
    res.reset(new Source(SmallImm((int) src.raddr, false)));
  } else {               // add b, rf
    res.reset(new Source(RFAddress(src.raddr)));
  }

  assert(res);
  return res;
#endif
}


std::unique_ptr<Source> Instr::mul_alu_a() const {
	auto const &src = alu.mul.a;

#if USE_MESA == 1
	return alu_src(src);
#else
  std::unique_ptr<Source> res;

  if (sig.small_imm_c) { // mul a, small imm
    res.reset(new Source(SmallImm((int) src.raddr, false)));
  } else {               // mul a, rf
    res.reset(new Source(RFAddress(src.raddr)));
  }

  assert(res);
  return res;
#endif
}


std::unique_ptr<Source> Instr::mul_alu_b() const {
	auto const &src = alu.mul.b;

#if USE_MESA == 1
	return alu_src(src);
#else
  std::unique_ptr<Source> res;

  if (sig.small_imm_d) { // mul b, small imm
    res.reset(new Source(SmallImm((int) src.raddr, false)));
  } else {               // mul b, rf
    res.reset(new Source(RFAddress(src.raddr)));
  }

  assert(res);
  return res;
#endif
}


std::unique_ptr<Source> Instr::alu_src(v3d_qpu_mux src) const {
	assert(!Platform::compiling_for_vc7());
  std::unique_ptr<Source> res;

  if (src < V3D_QPU_MUX_A) {
    // Accumulator
    res.reset(new Source(Register("", (v3d_qpu_waddr) src, src)));
  } else if (src == V3D_QPU_MUX_A) { // address a, rf-reg
    res.reset(new Source(RFAddress(raddr_a)));
  } else if (sig.small_imm_b) {      // address b, small imm
    res.reset(new Source(SmallImm((int) raddr_b, false)));
  } else {
    // address b, rf-reg
    res.reset(new Source(RFAddress(raddr_b)));
  }

  assert(res);
  return res;
}

#if 0

// mesa2
std::unique_ptr<Source> Instr::alu_src(v3d_qpu_input input) const {
  std::unique_ptr<Source> res;

  if (sig.small_imm_b) {
    // address b, small imm
    res.reset(new Source(SmallImm((int) raddr_b, false)));
  } else {
    // address b, rf-reg
    res.reset(new Source(RFAddress(raddr_b)));
  }

  assert(res);
  return res;
}

#endif

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

}  // namespace v3d
}  // namespace V3DLib
