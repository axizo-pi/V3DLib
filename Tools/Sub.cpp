#include <V3DLib.h>
#include "vc4/Instr.h"
#include "global/log.h"

using namespace V3DLib;
using namespace Log;

namespace {

void kernel(Int x, Int::Ptr result) {
	*result = x;
}


MAYBE_UNUSED void run_kernel() {
  auto k = compile(kernel);
  //cout << k.dump();
  cout << vc4::opcodes(k.code());
  //k.setNumQPUs(settings.num_qpus);

	Int::Array result(16);

	k.load(2, &result);
	k.run();

  std::string ret;
  for (int i = 0; i < (int) result.size(); i++) {
    ret << result[i] << ", ";
  }

  cout << "result: " << ret;
}

void vc4_branch() {
  vc4::Instr instr1;
  instr1.enc       = vc4::Instr::BRANCH_ENC;
  instr1.sig       = vc4::Instr::BRANCH;
  //instr1.rel       = true;
  instr1.reg       = true;
  instr1.raddr_a   = 8;
  instr1.immediate = 1234;
  instr1.waddr_add = 7;    // value 0 registers as NOP
  instr1.waddr_mul = 8;    // value 0 registers as NOP
	std::cout << "  " << instr1.dump() << "\n";

  vc4::Instr instr2;
  instr2.enc       = vc4::Instr::LOAD_IMM;
  instr2.immediate = 0x1234;
  //instr2.ws        = true;  // If false, waddr_add writes to rega and waddr_mul writes to regb
                            // If true, rega and regb are reversed
  instr2.waddr_add = 12;    // value 0 registers as NOP
  instr2.waddr_mul = 11;    // idem
	std::cout << "  " << instr2.dump() << "\n";

  std::vector<uint64_t> code;
  code.push_back(instr1.encode());
  code.push_back(instr2.encode());

	std::cout << "\nEncoding:\n";
	for (auto const &enc : code) {
		std::cout << "  " << std::hex << "0x" << enc << "\n";
	}

	std::cout << "\nOpcodes:\n" << vc4::opcodes(code);
}

} // anon namespace


int main(int argc, const char *argv[]) {
  //run_kernel();
	vc4_branch();

  return 0;
}
