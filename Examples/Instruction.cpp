#include "v3d/instr/v3d_api.h"
#include "broadcom/qpu/qpu_disasm.h"
#include <string>
#include <iostream>
#include <sstream>

using namespace std;


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


void display(struct v3d_qpu_instr const &instr) {
	cout <<
"\n"
"struct v3d_qpu_instr {\n"
"  type     : " << instr.type                       << ";\n"
"  sig      :"  << disp_sig(instr.sig)              << ";\n"
"  sig_addr : " << ((unsigned) instr.sig_addr)      << ";\n"
"  sig_magic: " << b(instr.sig_magic)               << ";\n"  /* If the signal writes to a magic address */
"  raddr_a  : " << ((unsigned) instr.raddr_a)       << ";\n"                      /* V3D 4.x */
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

int main(int argc, const char *argv[]) {
	struct v3d_qpu_instr instr = {
		.type = V3D_QPU_INSTR_TYPE_ALU,
		.sig  = { 0 /* .thrsw = 1 */ },
		.raddr_a = 0,
		0
	};

	display(instr);

	uint64_t packed =  instr_pack(&instr);

	cout << "\n";
	cout << "Packed: " << hex << packed << "\n";
	cout << "Decode: " << qpu_decode(&instr) << "\n";
	cout << "Disasm: " << qpu_disasm(packed) << "\n";

  return 0;
}
