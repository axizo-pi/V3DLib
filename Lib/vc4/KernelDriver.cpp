#include "KernelDriver.h"
#include <iostream>
#include <sstream>
#include "Source/Lang.h"
#include "Source/Translate.h"
#include "Target/RemoveLabels.h"
#include "vc4.h"
#include "Target/instr/Mnemonics.h"
#include "SourceTranslate.h"  // add_uniform_pointer_offset()
#include "Target/Satisfy.h"
#include "RegAlloc.h"
#include "global/log.h"
#include "Instr.h"
#include "LibSettings.h"
#include "Support/Helpers.h"
#include "Functions.h"

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
  assert(m_code.allocated());
  return m_code.size();
}


/**
 * Encode target instructions
 *
 * Assumption: code in a kernel, once allocated, does not change.
 */
void KernelDriver::encode() {
  if (!m_code.empty()) return;  // Don't bother if already encoded
  if (has_errors()) return;         // Don't do this if compile errors occured

  CodeList code = V3DLib::vc4::encode(m_targetCode);

  // Allocate memory for QPU code
  m_code.alloc(code.size());
  assert(m_code.size() > 0);

  // Copy kernel to code memory
  int offset = 0;
  for (int i = 0; i < code.size(); i++) {
    m_code[offset++] = code[i];
  }
}


std::string KernelDriver::emit_opcodes() {
  encode();

  auto list = vc4::opcodes(m_code);

  // Following takes tags INIT_BEGIN/INIT_END into account
  if ((int) (list.size() + 2) != m_targetCode.size()) {
    Log::cerr << "vc4 emit_opcodes() discrepancy in opcode and target code size."
              << "opcode size: " << list.size() << " (plus INIT), "
              << "target code size: " << m_targetCode.size()
              << thrw
    ;
  }

  int max_size = 0;
  for (int i = 0; i < (int) list.size(); ++i) {
    if (max_size < (int) list[i].size()) {
      max_size = (int) list[i].size();
    }
  }

  std::string ret;
  for (int i = 0; i < (int) list.size(); ++i) {
    auto &t = m_targetCode[i];

    if (t.tag == INIT_BEGIN || t.tag == INIT_END) {
      warn << "emit_opcodes() detected INIT marker";
    }

    ret << t.emit_header();

    if (LibSettings::dump_line_numbers()) {
      ret << i << ": ";
    }

    ret << list[i]
        << t.emit_comment((int) list[i].size(), max_size)
        << "\n";
  }

  return ret;
}


void KernelDriver::compile_intern() {
  vc4::kernelFinish();

  // NOTE During debugging, I noticed that the sequence on the statement stack is duplicated here.
  //      I can not discover why, it's benevolent, it's not clean but I'm leaving it for now.
  // TODO Fix it one day (sigh)

  obtain_ast();

  V3DLib::translate_stmt(m_targetCode, m_body);
  assert(!m_targetCode.empty());
  insertInitBlock(m_targetCode);

  // Add final dummy uniform handling - See Note 1, function `invoke()` in `vc4/Invoke.cpp`,
  {
    using namespace V3DLib::Target::instr;  // for mov()

    Reg tmp1 = VarGen::fresh();

    Target::Instr::List ret;
    ret << mov(tmp1, Var(UNIFORM));
    ret.front().comment("Last uniform load is dummy value");

    int index = m_targetCode.lastUniformOffset();
    assert(index > 0);
    m_targetCode.insert(index + 1, ret);
  }

  vc4::add_init_block(m_targetCode);

  m_targetCode << Target::Instr(END);
  compile_postprocess(m_targetCode);
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

  MailBoxInvoke::invoke(numQPUs, m_code, params);
}

}  // namespace vc4
}  // namespace V3DLib

