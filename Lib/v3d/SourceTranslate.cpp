#include "SourceTranslate.h"
#include "Support/basics.h"
#include "Support/Timer.h"
#include "Source/Translate.h"
#include "Source/Stmt.h"
#include "Liveness/Liveness.h"
#include "Target/Subst.h"
#include "vc4/DMA/DMA.h"
#include "Target/instr/Mnemonics.h"
#include "Common/CompileData.h"
#include <iostream>

namespace V3DLib {

using ::operator<<;  // C++ weirdness

namespace v3d {

Instr::List SourceTranslate::store_var(Var dst_addr, Var src) {
  using namespace V3DLib::Target::instr;
  Instr::List ret;

  Reg srcData(dst_addr);
  Reg srcAddr(src);

  // NOTE: these are instructions from Target, not v3d/instr
  ret << mov(TMUD, srcAddr)
      << mov(TMUA, srcData)
      << tmuwt();

  ret.front().comment("store_var v3d");

  return ret;
}


void SourceTranslate::regAlloc(Instr::List &instrs) {
  int numVars = VarGen::count();

  Liveness::optimize(instrs, numVars);

  // Step 0 - Perform liveness analysis
  Liveness live(numVars);
  live.compute(instrs);

  // Step 2 - For each variable, determine all variables ever live at the same time
  LiveSets liveWith(numVars);
  liveWith.init(instrs, live);

  // Step 3 - Allocate a register to each variable
  for (int i = 0; i < numVars; i++) {
    if (live.reg_usage()[i].reg.tag != NONE) continue;

    auto possible = liveWith.possible_registers(i, live.reg_usage());

    live.reg_usage()[i].reg.tag = REG_A;
    RegId regId = LiveSets::choose_register(possible, false);

    if (regId < 0) {
      std::string buf = "v3d regAlloc(): register allocation failed for target instruction ";
      buf << i << ": " << instrs[i].mnemonic();
      cerr << buf << thrw;
    } else {
      live.reg_usage()[i].reg.regId = regId;
    }
  }

  compile_data.allocated_registers_dump = live.reg_usage().dump(true);

  // Step 4 - Apply the allocation to the code
  allocate_registers(instrs, live.reg_usage());
}


Instr label(Label in_label) {
  Instr instr;
  instr.tag = LAB;
  instr.label(in_label);

  return instr;
}


/**
 * @brief Add initialization code after uniform loads
 */
void add_init_block(Instr::List &code) {
  using namespace V3DLib::Target::instr;

  Reg acc  = ACC0();
  Reg _r64 = _64();

  Instr::List ret;

  if (Platform::compiling_for_vc7()) {
    // vc7: No restriction on #QPU's 1 or 8
    ret << mov(acc, QPU_ID)
        << shr(acc, acc, 2)
        << band(rf(RSV_QPU_ID), acc, 15)
    ;
  } else {
    // vc6: Determine the qpu index for 'current' QPU
    // This is derived from the thread index. 
    //
    // Broadly:
    //
    // If (numQPUs() == 8)  // Alternative is 1, then qpu num initalized to 0 is ok
    //   me() = (thread_index() >> 2) & 0b1111;
    // End
    //
    // This works because the thread indexes are consecutive for multiple reserved
    // threads. It's probably also the reason why you can select only 1 or 8 (max)
    // threads, otherwise there would be gaps in the qpu id.
    //
    Label endifLabel = freshLabel();

    ret << mov(rf(RSV_QPU_ID), 0)           // not needed, already init'd to 0. Left here to counter future brainfarts
        << sub(acc, rf(RSV_NUM_QPUS), 8).pushz()
        << branch(endifLabel).allzc()       // nop()'s added downstream
        << mov(acc, QPU_ID)
        << shr(acc, acc, 2)
        << band(rf(RSV_QPU_ID), acc, 15)
        << label(endifLabel)
    ;
  }

  ret << mov(_r64, 1)
      << shl(_r64, _r64, 6)   . comment("init global 64");

  ret << add_uniform_pointer_offset(code);

  insert_init_block(code, ret);
}


/**
 * Process statements which are specific for v3d.
 *
 * For v3d, the only thing that happens is that DMA is blocked
 * <i>(which feels a bit useless IMHO)</i>.
 *
 * @return true if statement handled, false otherwise
 */
bool SourceTranslate::stmt(Instr::List &seq, Stmt::Ptr s) {
  if (DMA::Stmt::is_dma_tag(s->tag)) {
    fatal("VPM and DMA reads and writes can not be used for v3d");
    return true;
  }

  return false;
}

}  // namespace v3d
}  // namespace V3DLib
