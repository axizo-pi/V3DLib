#include "KernelDriver.h"
#include <iostream>
#include <sstream>
#include "Source/Lang.h"
#include "Source/Translate.h"
#include "Target/RemoveLabels.h"
#include "vc4.h"
#include "DMA/Operations.h"
#include "Target/instr/Mnemonics.h"
#include "SourceTranslate.h"  // add_uniform_pointer_offset()
#include "Target/Satisfy.h"
#include "RegAlloc.h"
#include <stdio.h>
#include <stdlib.h>
#include "global/log.h"
#include "Instr.h"

using namespace Log;

namespace V3DLib {

using ::operator<<;  // C++ weirdness
using namespace std;

namespace vc4 {

namespace {


/**
 * vc4 LDTMU implicitly writes to ACC4, take this into account
 */
void loadStorePass(Target::Instr::List &instrs) {
  using namespace V3DLib::Target::instr;

  Target::Instr::List newInstrs(instrs.size()*2);

  for (int i = 0; i < instrs.size(); i++) {
    Target::Instr instr = instrs[i];

		auto acc4 = ACC4();   // Should not be converted to rf here

    if (instr.tag == RECV && instr.dest() != acc4) {
      Target::Instr::List tmp(2);
      tmp << recv(acc4)
          << mov(instr.dest(), acc4);
      tmp.front().transfer_comments(instr);

      newInstrs << tmp;
      continue;
    }

    newInstrs << instr;
  }

  // Update original instruction sequence
  instrs.clear();
  instrs << newInstrs;
}


/**
 * @param targetCode  output variable for the target code assembled from the AST and adjusted
 */
void compile_postprocess(Target::Instr::List &targetCode) {
  assertq(!targetCode.empty(), "compile_postprocess(): passed target code is empty");

	loadStorePass(targetCode);
  //compile_data.target_code_before_regalloc = targetCode.dump();

  // Perform register allocation
  regAlloc(targetCode);

  // Satisfy target code constraints
  vc4_satisfy(targetCode);
}


}  // anon namespace


KernelDriver::KernelDriver() : V3DLib::KernelDriver(Vc4Buffer) {
	assert(Platform::compiling_for_vc4());
	Log::debug << "selecting vc4 as kernel type";
	m_type = vc4;
}


int KernelDriver::kernel_size() const {
  assert(qpuCodeMem.allocated());
  return qpuCodeMem.size();
}


/**
 * Add the postfix code to the kernel.
 *
 * Note that this emits kernel code.
 */
void KernelDriver::kernelFinish() {
  dmaWaitRead();                header("Kernel termination");
                                comment("Ensure outstanding DMAs have completed");
  dmaWaitWrite();

  If (me() == 0)
    Int n = numQPUs()-1;        comment("QPU 0 wait for other QPUs to finish");
    For (Int i = 0, i < n, i++)
      semaDec(15);
    End
    hostIRQ();                  comment("Send host IRQ");
  Else
    semaInc(15);
  End
}


/**
 * Encode target instructions
 *
 * Assumption: code in a kernel, once allocated, does not change.
 */
void KernelDriver::encode() {
  if (!qpuCodeMem.empty()) return;  // Don't bother if already encoded
  if (has_errors()) return;         // Don't do this if compile errors occured

  CodeList code = V3DLib::vc4::encode(m_targetCode);

  // Allocate memory for QPU code
  qpuCodeMem.alloc(code.size());
  assert(qpuCodeMem.size() > 0);

  // Copy kernel to code memory
  int offset = 0;
  for (int i = 0; i < code.size(); i++) {
    qpuCodeMem[offset++] = code[i];
  }
}


std::string KernelDriver::emit_opcodes() {
  encode();
  return vc4::opcodes(qpuCodeMem);
}


void KernelDriver::compile_intern() {
  kernelFinish();

  // NOTE During debugging, I noticed that the sequence on the statement stack is duplicated here.
  //      I can not discover why, it's benevolent, it's not clean but I'm leaving it for now.
  // TODO Fix it one day (sigh)

  obtain_ast();

  V3DLib::translate_stmt(m_targetCode, m_body);

  {
    using namespace V3DLib::Target::instr;  // for mov()
    // Add final dummy uniform handling - See Note 1, function `invoke()` in `vc4/Invoke.cpp`,
    // and uniform ptr index offsets

    // _64 not working well on vc4
    //Reg _r64 = _64();
    Reg tmp1 = VarGen::fresh();
    //Reg tmp2 = VarGen::fresh();

    Target::Instr::List ret;
    ret << mov(tmp1, Var(UNIFORM))
        << add_uniform_pointer_offset(m_targetCode);  // !!! NOTE: doesn't take dummy in previous into account
                                                      // This should not be a problem
/*
        << mov(tmp2,1)                                // Init global 64
        << shl(_r64, tmp2, 6);
*/
		ret.front().comment("Last uniform load is dummy value");

    int index = m_targetCode.lastUniformOffset();
    assert(index > 0);
    m_targetCode.insert(index + 1, ret);
  }

  m_targetCode << Target::Instr(END);

  compile_postprocess(m_targetCode);

  // Translate branch-to-labels to relative branches
  removeLabels(m_targetCode);

  encode();
}


void KernelDriver::invoke(int numQPUs, IntList &params, bool wait_complete) {
  assert(params.size() != 0);

  if (handle_errors()) {
    fatal("Errors during kernel compilation/encoding, can't continue.");
  }

	if (!wait_complete) {
		warn << "run(): disabling wait completion only works for v3d. Ignoring for vc4.";
	}

  MailBoxInvoke::invoke(numQPUs, qpuCodeMem, params);
}

}  // namespace vc4
}  // namespace V3DLib

