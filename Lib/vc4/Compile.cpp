#include "Compile.h"
#include "../CodeStruct.h"
#include "../LibSettings.h"
#include "Functions.h"
#include "Instr.h"
#include "Source/Translate.h"
#include "SourceTranslate.h"
#include "Target/RemoveLabels.h"
#include "Encode.h"
#include "Target/Satisfy.h"
#include "Target/instr/Mnemonics.h"
#include "RegAlloc.h"

namespace V3DLib {
namespace vc4 {

using ::operator<<;  // C++ weirdness

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

  regAlloc(targetCode);      // Perform register allocation
  vc4_satisfy(targetCode);   // Satisfy target code constraints
}

}  // anon namespace

Compile::Compile() {
  assert(Platform::compiling_for_vc4());
  Log::debug << "selecting vc4 as kernel type";
  m_type = vc4;

  init_compile();
}


int Compile::kernel_size() const {
	auto const &code = code_struct().m_code;
  assert(code.allocated());
  return code.size();
}


/**
 * @brief Encode target instructions
 *
 * Assumption: code in a kernel, once allocated, does not change.
 */
void Compile::encode() {
	auto &cs = code_struct();

  if (!cs.m_code.empty()) return;      // Don't bother if already encoded
  if (has_errors())    return;      // Don't do this if compile errors occured

  CodeList code = V3DLib::vc4::encode(cs.m_targetCode);

  // Allocate memory for QPU code
  cs.m_code.alloc(code.size());
  assert(cs.m_code.size() > 0);

  // Copy kernel to code memory
  int offset = 0;
  for (int i = 0; i < code.size(); i++) {
    cs.m_code[offset++] = code[i];
  }
}


void Compile::invoke(int numQPUs, IntList &params, bool wait_complete) {
  if (has_errors()) {
    fatal("Errors during kernel compilation/encoding, can't continue.");
  }

	auto &cs = code_struct();
  m_driver.invoke(cs.m_code, numQPUs, params,  wait_complete);
}


void Compile::compile_intern() {
	auto &cs = code_struct();
  vc4::kernelFinish();

  cs.obtain_ast();
  encode_source(cs.m_targetCode, cs.m_body);

  // Add final dummy uniform handling - See Note 1, function `invoke()` in `vc4/Invoke.cpp`,
  {
    using namespace V3DLib::Target::instr;  // for mov()

    Reg tmp1 = VarGen::fresh();

    Target::Instr::List ret;
    ret << mov(tmp1, Var(UNIFORM));
    ret.front().comment("Last uniform load is dummy value");

    int index = cs.m_targetCode.lastUniformOffset();
    assert(index > 0);
    cs.m_targetCode.insert(index + 1, ret);
  }

  vc4::add_init_block(cs.m_targetCode);
  cs.m_targetCode << Target::Instr(END);

  compile_postprocess(cs.m_targetCode);

  removeLabels(cs.m_targetCode);

  encode();
}


std::string Compile::emit_opcodes() {
	auto &cs = code_struct();
  encode();

  auto list = vc4::opcodes(cs.m_code);

  // Following takes tags INIT_BEGIN/INIT_END into account
  if ((int) (list.size() + 2) != cs.m_targetCode.size()) {
    Log::cerr << "vc4 emit_opcodes() discrepancy in opcode and target code size. "
              << "opcode size: " << list.size() << " (plus INIT), "
              << "target code size: " << cs.m_targetCode.size()
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
  int t_i = 0;
  for (int i = 0; i < (int) list.size(); ++i, ++t_i) {
    auto t = cs.m_targetCode[t_i];

    if (t.tag == INIT_BEGIN || t.tag == INIT_END) {
      cdebug << "emit_opcodes() detected INIT marker";
      ++t_i;
      t = cs.m_targetCode[t_i];
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

} // namespace V3DLib
} // namespace vc4
