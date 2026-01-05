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
  //cdebug << k.dump();
  cdebug << vc4::opcodes(k.code());
  //k.setNumQPUs(settings.num_qpus);

	Int::Array result(16);

	k.load(2, &result);
	k.run();

  std::string ret;
  for (int i = 0; i < (int) result.size(); i++) {
    ret << result[i] << ", ";
  }

  warn << "result: " << ret;
}

} // anon namespace


int main(int argc, const char *argv[]) {
  //run_kernel();

  vc4::Instr instr1;
  instr1.enc       = vc4::Instr::BRANCH_ENC;
  instr1.sig       = vc4::Instr::BRANCH;
  //instr1.rel       = true;
  instr1.reg       = true;
  instr1.raddr_a   = 8;
  instr1.immediate = 1234;
  instr1.waddr_add = 7;    // value 0 registers as NOP
  instr1.waddr_mul = 8;    // value 0 registers as NOP
  cdebug << instr1.dump();

  vc4::Instr instr2;
  instr2.enc       = vc4::Instr::LOAD_IMM;
  instr2.immediate = 0x1234;
  //instr2.ws        = true;  // If false, waddr_add writes to rega and waddr_mul writes to regb
                            // If true, rega and regb are reversed
  instr2.waddr_add = 12;    // value 0 registers as NOP
  instr2.waddr_mul = 11;    // idem
  cdebug << instr2.dump();

  std::vector<uint64_t> code;
  code.push_back(instr1.encode());
  code.push_back(instr2.encode());
  warn << "Opcodes:\n" << vc4::opcodes(code);

  return 0;
}
