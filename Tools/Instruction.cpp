/**
 * This is a dirty hack tool to determine the binary representation of V3D instructions.
 *
 * ===============================================================================================
 * NOTES
 * =====
 *
 * ALU
 * ===
 * - waddr and a/b raddr for both add and mul 6 bits
 * - mul raddr's: order preserved
 * - sig_addr does not register, maybe dependent on sig
 * - various sig's can not be combined; not all combinations are valid
 *   - following verified ok: thrsw ldunif
 * - mul add output_pack can not be disasm'd for nonzero values
 * - faddnf: translates to fadd when packed
 * - add.op = V3D_QPU_A_VFPACK sets 'add output pack' to 11. Otherwise it's just an V3D_QPU_A_FADD
 *
 * 4x
 * --
 * - Can't set add/mul.a/b.raddr (expected)
 * - a/b might be flipped on add.fadd, mul.add
 *
 * 7x
 * --
 * - raddr_a, raddr_b do not register (expected)
 * - add raddr's: smallest value always put in a from instr struct (check 4x?)
 * - sig, can not combine:
 *		- thrsw, small_imm_a
 * - When a small_imm field is set, set corresponding raddr is a small immediate
 *    - int range: -16..15
 * - Setting 2 fields to small_imm results in assertion on disasm.
 * - 'mov rf0, 0' needs to use add.a raddr for small_imm
 *
 *
 * BRANCH
 * ======
 *
 * - Adresses in instr get rounded down to 4-bit boundaries on pack.
 *   This makes sense since instructions are 64 bits.
 */
#include "v3d/instr/v3d_api.h"
#include "broadcom/qpu/qpu_disasm.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

#define MAYBE_UNUSED __attribute__((unused))

namespace {

string line = 
"_________________________________________________________________________________________________\n";

string h =
"|63|62|61|60|59|58|57|56|55|54|53|52|51|50|49|48|47|46|45|44|43|42|41|40|39|38|37|36|35|34|33|32|\n"
"|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|\n"
	;

string h2_temp =
"|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |\n"
"|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |\n"
	;

std::string format_bits(uint64_t val) {
	stringstream buf;

	for (int n = 63; n >= 0; n--) {
		int bit = (val >> n) & 1;

		buf << "|" << setw(2) << bit;

		if (n == 32) buf << "|\n";
	}
	buf << "|\n";

	return buf.str();
}


MAYBE_UNUSED std::string diff_bits(uint64_t a, uint64_t b) {
	int count = 0;
	stringstream buf;

	buf << line;

	for (int n = 63; n >= 0; n--) {
		int bit_a = (a >> n) & 1;
		int bit_b = (b >> n) & 1;

		if (bit_a == bit_b) {
			buf << "|  ";
		} else {
			count++;
			if (bit_a == 1) {
				buf << "| a";
			} else {
				buf << "| b";
			}
		}

		if (n == 32) buf << "|\n";
	}
	buf << "|\n";

	buf << line;

	stringstream buf2;
	buf2 << "\n" << count << " diffs\n";

	return buf2.str() + buf.str();
}


string instr_format(string h2, string desc, uint64_t val, bool show_desc = false) {
	stringstream buf;

	buf << line << h << line << h2 << line << format_bits(val) << line;
	if (show_desc) {
		buf << desc;
	}

	return buf.str();
}

} // anon namespace


std::string b(bool val) {
	return val?"true":"false";
}


std::string disp_flags(struct v3d_qpu_flags const &f) {
	stringstream buf;

	buf
		<< "ac : " << f.ac  << "; mc : " << f.mc  << "; "
		<< "apf: " << f.apf << "; mpf: " << f.mpf << "; "
		<< "auf: " << f.auf << "; muf: " << f.muf
	;

	return buf.str();
}


std::string disp_sig(struct v3d_qpu_sig const &sig) {
	stringstream buf;

  if (sig.thrsw) buf << " thrsw";
  if (sig.ldunif) buf << " ldunif";
  if (sig.ldunifa) buf << " ldunifa";
  if (sig.ldunifrf) buf << " ldunifrf";
  if (sig.ldunifarf) buf << " ldunifarf";
  if (sig.ldtmu) buf << " ldtmu";
  if (sig.ldvary) buf << " ldvary";
  if (sig.ldvpm) buf << " ldvpm";
  if (sig.ldtlb) buf << " ldtlb";
  if (sig.ldtlbu) buf << " ldtlbu";
  if (sig.ucb) buf << " ucb";
  if (sig.rotate) buf << " rotate";
  if (sig.wrtmuc) buf << " wrtmuc";
  if (sig.small_imm_a) buf << " small_imm_a"; /* raddr_a (add a), since V3D 7.x */
  if (sig.small_imm_b) buf << " small_imm_b"; /* raddr_b (add b) */
  if (sig.small_imm_c) buf << " small_imm_c"; /* raddr_c (mul a), since V3D 7.x */
  if (sig.small_imm_d) buf << " small_imm_d"; /* raddr_d (mul b), since V3D 7.x */

	return buf.str();
}


std::string disp_input(struct v3d_qpu_input const &n) {
	stringstream buf;

	buf << "{ " 
    //enum v3d_qpu_mux mux; // V3D 4.x
    << "raddr: "    << ((unsigned) n.raddr) // V3D 7.x
		<< ", unpack: " << n.unpack
		<< "}";

	return buf.str();
}


std::string disp_alu_instr(struct v3d_qpu_alu_instr const &n) {
	stringstream buf;

	buf <<
"  {\n"
"    add {\n"
"      op         : " << n.add.op                 << ";\n"
"      a          : " << disp_input(n.add.a)      << ";\n"
"      b          : " << disp_input(n.add.b)      << ";\n"
"      waddr      : " << ((unsigned) n.add.waddr) << ";\n"
"      magic_write: " << n.add.magic_write        << ";\n"
"      output_pack: " << n.add.output_pack        << ";\n"
"    };\n"
"\n"
"    mul {\n"
"      op         : " << n.mul.op                 << ";\n"
"      a          : " << disp_input(n.mul.a)      << ";\n"
"      b          : " << disp_input(n.mul.b)      << ";\n"
"      waddr      : " << ((unsigned) n.mul.waddr) << ";\n"
"      magic_write: " << n.mul.magic_write        << ";\n"
"      output_pack: " << n.mul.output_pack        << ";\n"
"    }\n"

"  }\n";
	return buf.str();
}


std::string disp_branch_instr(struct v3d_qpu_branch_instr const &b) {
	stringstream buf;

	buf << "TODO\n";

	return buf.str();
}


void display_instr(struct v3d_qpu_instr const &instr) {
	cout <<
"\n"
"struct v3d_qpu_instr {\n"
"  type     : " << instr.type                       << ";\n"
"  sig      :"  << disp_sig(instr.sig)              << ";\n"
"  sig_addr : " << ((unsigned) instr.sig_addr)      << ";\n"
"  sig_magic: " << b(instr.sig_magic)               << ";\n"  /* If the signal writes to a magic address */
"  raddr_a  : " << ((unsigned) instr.raddr_a)       << "; (4.x)\n"                      /* V3D 4.x */
"  raddr_b  : " << ((unsigned) instr.raddr_b)       << ";\n"                      /* V3D 4.x (holds packed small immediate in 7.x too) */
"  flags    : " << disp_flags(instr.flags)          << ";\n"
"\n";

	if (instr.type == V3D_QPU_INSTR_TYPE_ALU) {
		cout << disp_alu_instr(instr.alu);
	} else {
		cout << disp_branch_instr(instr.branch);
	}

	cout << "};\n" ;
}


std::string instr_format_alu_7x(uint64_t val) {

	string h2 =
"|  |  |  |  |  | 1| sig          |sm| sig_addr (2apf?)|mw|aw| mul waddr       | add waddr       |\n"
"|  |  | a_pa|  |1?|  |1?| mul a raddr     | mul b raddr     | add a raddr     | add b raddr     |\n"
	;

	string desc = "\n"
		"- mw  : mul magic write\n"
		"- aw  : add magic write\n"
		"- a_pa: add output pack\n"
		"- ac: flags\n"
		"  - 51: condition enabled\n"
		"  - 50: 0\n"
		"  - 49: negate condition\n"
		"  - 48: 0=a, 1=b\n"
		"- apf?: there is overlap with mpf. I don't get it yet\n"
		"  - It looks like apf/mpf share the same field, and bit 50 is set for mul\n"
		"    I get the impression that add/mul exclude each other on push\n"
		"- sm  : sig_magic; true handles sig_addr as v3d_qpu_waddr\n"
		"- sig:\n"
		"                   53: thrsw \n"
		"               54    : ldunif    - combines with thrsw; can't combine with ldunifa, ldunifrf, ldunifarf\n"
		"   57, 56            : ldunifa   - can't combine with thrsw\n"
		"       56, 55        : ldunifrf  - with sig_addr\n"
		"   57, 56,         53: ldunifarf\n"
		"           55        : ldtmu\n"
		"       56            : ldvary\n"
		"                     : ldvpm     - error disasm when used on its own\n"
		"   57                : ldtlb\n"
		"   57,             53: ldtmu\n"
		"   57,     55, 54    : ucb\n"
		"                     : rotate    - error disasm when used on its own\n"
		"   57,         54    : wrtmuc\n"
		"\n"
		"       56, 55, 54    : small_imm_a - 0 in raddr does not register for small_imm's\n"
		"       56, 55, 54, 53: small_imm_b\n"
		"   57, 56, 55, 54    : small_imm_c\n"
		"   57, 56, 55, 54, 53: small_imm_d\n"
	;


	return instr_format(h2, desc, val);
}


std::string instr_format_branch_7x(uint64_t val) {

	string h2 =
"|  |  |  |  |  | 0| 1|  | offset_bottom                                                | cond   |\n"
"| offset_top            |  | fign|  |  |  |  | bdu |ub| bdi | raddr_a         |  |  |  |  |  |  |\n"
	;

	string desc = "\n"
		"- offset_top   : top 8 bits of 32-bit offset address\n"
		"- offset_bottom: lower 24 bits of 32-bit offset address. The bottom 3 bits are left out\n"
		"- fign         : msfign\n"
		"- cond         : enum values of cond are not consecutive in the packed instr\n"
		"- ub           : if set, adds 'a:unif' to mnemonic\n"
		"- bdu          : only set if ub set. a:absolute, r:relative, lri:link regfile, rf: regfile\n"
		"- offset       : 16-bit aligned. The bottom nibble is rounded downward.\n"
		"                 not set if bdi = _REGFILE\n"
		"- raddr_a      : only filled in if bdi == _REGFILE\n"
	;

	return instr_format(h2, desc, val, true);
}


std::string instr_format_alu_4x(uint64_t val) {

	string h2 =
"|  |  |  |  |  | 1| 0|  |  |  |  |  |  |  |  |  |  |  |  |  | mul_waddr       | add_waddr       |\n"
"|  |  |  |  |  |1?|  |1?|mul_muxb|mul_muxa|add_muxb|add_muxa| raddr_a         | raddr_b         |\n"
	;

	string desc = "\n"
	;


	return instr_format(h2, desc, val);
}


std::string instr_format_branch_4x(uint64_t val) {

	string h2 =
"|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |\n"
"| offset_top            |  | fign|  |  |  |  | bdu |ub| bdi |  |  |  |  |  |  |  |  |  |  |  |  |\n"
	;

	string desc = "\n"
	;

	return instr_format(h2, desc, val);
}


int main(int argc, const char *argv[]) {
	MAYBE_UNUSED struct v3d_qpu_instr instr_alu = {
		.type = V3D_QPU_INSTR_TYPE_ALU,
		.sig  = {
			//.thrsw = 1,
			//.ldunifrf = 1
			//.ldtmu = 1,
			//.wrtmuc = 1
			.small_imm_b = 1,
	 	},
		.sig_addr = 0,
		.sig_magic = false,
    .raddr_a = 3,  // 4x
    .raddr_b = 0,  // 4x
		.flags = {
		  //.ac = V3D_QPU_COND_IFA,
		  //.mc = V3D_QPU_COND_IFA
			//.apf = V3D_QPU_PF_PUSHZ,
			//.mpf = V3D_QPU_PF_PUSHC,
		},
		.alu = {
			.add = {
				  .op = V3D_QPU_A_OR,
			  	.a = {
            .mux = V3D_QPU_MUX_B,
            //.raddr = 2 
          },
			  	.b = {
            .mux = V3D_QPU_MUX_B,
          },
		 	    .waddr = 13, //V3D_QPU_WADDR_NOP,
					.magic_write = false,
			},
			.mul = {
				  .op = V3D_QPU_M_ADD,
			  	.a = {
            .mux = V3D_QPU_MUX_R4,
            //.raddr = 0
          },
			  	.b = {
            .mux = V3D_QPU_MUX_B,
            //.raddr = 0
          },
				  //.op = V3D_QPU_M_FMOV,
					//.output_pack = V3D_QPU_PACK_NONE  // Other values fail disasm
		 	    .waddr = 10, //V3D_QPU_WADDR_TMUA,
					.magic_write = false,
			},
		},
	};

	MAYBE_UNUSED struct v3d_qpu_instr instr_branch = {
		.type = V3D_QPU_INSTR_TYPE_BRANCH,
		.branch = {
			//.cond = V3D_QPU_BRANCH_COND_ANYNA,
			//.msfign = V3D_QPU_MSFIGN_Q,

			//.bdi = V3D_QPU_BRANCH_DEST_REL,
			.bdi = V3D_QPU_BRANCH_DEST_REGFILE,

      .bdu = V3D_QPU_BRANCH_DEST_REL,

			.ub = false,
			.raddr_a = 33,
			.offset = 0x10
		}
	};

	auto &instr = instr_branch;

	//display_instr(instr);

	uint64_t packed =  instr_pack(&instr);

	cout << "\n";
	cout << "Packed: " << hex << packed << "\n";
	cout << "Decode: " << qpu_decode(&instr) << "\n";
	cout << "Disasm: " << qpu_disasm(packed) << "\n";

  if (devinfo_ver() == 42) {
  	if (instr.type == V3D_QPU_INSTR_TYPE_ALU) {
  		cout << instr_format_alu_4x(packed);
  	} else {
  		cout << instr_format_branch_4x(packed);
  	}
  } else {
  	if (instr.type == V3D_QPU_INSTR_TYPE_ALU) {
  		cout << instr_format_alu_7x(packed);
  	} else {
  		cout << instr_format_branch_7x(packed);
  	}
  }

/*
  uint64_t const prev = 0x38000000f903f003;  
	cout << "\n\nPrev  : " << hex << prev << "\n";
	cout << "Disasm: " << qpu_disasm(prev) << "\n";
	cout << diff_bits(prev, packed);
*/
  return 0;
}
