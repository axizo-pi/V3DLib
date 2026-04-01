#include "Emulator.h"
#include "Debugger.h"
#include <cmath>
#include "global/log.h"
#include "EmuSupport.h"
#include "Mutex.h"
#include "State.h"
#include "DMA.h"
#include "Target/SmallLiteral.h"
#include "Target/BufferObject.h"

namespace V3DLib {

using namespace Target;
using namespace Log;

namespace {

/**
 *
 * -----------------------------
 *
 * Note 1
 * ------
 *
 * Previously, the handling of `SPECIAL_DMA_LD_WAIT` and `SPECIAL_DMA_ST_WAIT`
 * was used to complete the DMA load/store.  
 * This is wrong and actually led to wrong results in the emulator.
 *
 * The DMA load/store operations actually run independent of the QPU execution.
 * If `SPECIAL_DMA_LD_WAIT`, `SPECIAL_DMA_ST_WAIT` are called while and load/store
 * is running, it should block on this operation until the DMA operation completes.
 */
Vec DMA_readReg(QPUState* s, State* g, Reg reg, bool &handled) {
  assert(reg.tag == SPECIAL);

  Vec v(0);
  handled = true;

  switch (reg.regId) {
    case SPECIAL_VPM_READ: {
      // Make sure there's a VPM load request waiting
      assert(!s->vpmLoadQueue.isEmpty());
      VPMLoadReq* req = s->vpmLoadQueue.first();
      vpm_read(*req, g->vpm, v);
      if (req->numVecs == 0) s->vpmLoadQueue.deq(); 
			//TODO check if needed: s->loadBuffer.push(v);
    }
    return v;

    //
    // Wait for DMA load to complete.
    // See Note 1
    //
    case SPECIAL_DMA_LD_WAIT: {
      if (s->dmaLoad.waiting()) {
        // This actually happens; see loadRequest() in LoadStore.cpp.
        s->waiting = true;
      }
      return v; // Return value unspecified
    }

    //
    // Wait for DMA store to complete.
    // See Note 1
    //
    case SPECIAL_DMA_ST_WAIT: {
      if (s->dmaStore.waiting()) {  // Also happens
        s->waiting = true;
      }
      return v; // Return value unspecified
    }

    default:
      handled = false;
      break;
  }

  return v;
}


Vec read_special_register(QPUState &s, State &g, Reg reg) {
  assert(reg.tag == SPECIAL);

  Vec v(0);  // Default is dummy value

  switch(reg.regId) {
    case SPECIAL_ELEM_NUM:
      return EmuState::index_vec;

    case SPECIAL_UNIFORM:
      assertq(false, "read_special_register(): not expecting SPECIAL_UNIFORM to be handled any more");
      break;

    case SPECIAL_QPU_NUM:
      return Vec(s.id);

    case SPECIAL_MUTEX_ACQUIRE:
      Mutex::acquire(s.id);
      break;

    default: {
      bool handled;
      v = DMA_readReg(&s, &g, reg, handled);
      if (handled) {
        return v; // Return value unspecified, don't care at this point
      }

      cerr << "Emulator: can't read special register: " << reg.dump() << thrw;
    }
  }

  return v;  // Dummy return value
}


/**
 * Read a vector register
 */
Vec readReg(QPUState &s, State &g, Reg const &reg) {
  int r = reg.regId;
  //warn << "readReg reg, r, tag: " << reg.dump() << ", " << r << ", " << reg.tag;
  Vec v(0);

  switch (reg.tag) {
    case NONE:
      return v;

    case REG_A:
      assert(r >= 0 && r < s.sizeRegFileA);
      return s.regFileA[r];

    case REG_B:
      assert(r >= 0 && r < s.sizeRegFileB);
      return s.regFileB[reg.regId];

    case ACC: {
      assert(r >= 0 && r <= 5);
      return s.accum[r];
    }

    case SPECIAL:
      return read_special_register(s, g, reg);

    default:
      break;  // Should not happen
  }

  // Should never be reached
  assert(false);
  return v;
}


// ============================================================================
// Check condition flags
// ============================================================================

/**
 * @brief Check the assignment condition
 *
 * Given an assignment condition and an vector index, determine if the
 * condition is true at that index using the implicit condition flags.
 */
inline bool checkAssignCond(QPUState* s, AssignCond cond, int i) {
  using Tag = AssignCond::Tag;

  switch (cond.tag) {
    case Tag::NEVER:  return false;
    case Tag::ALWAYS: return true;
    case Tag::FLAG:
      switch (cond.flag) {
        case ZS: return  s->zeroFlags[i];
        case ZC: return !s->zeroFlags[i];
        case NS: return  s->negFlags[i];
        case NC: return !s->negFlags[i];
      }
  }

  // Unreachable
  assert(false);
  return false;
}


/**
 * Given a branch condition, determine if it evaluates to true using
 * the implicit condition flags.
 */
inline bool checkBranchCond(QPUState* s, BranchCond cond) {
  bool bools[NUM_LANES];

  auto set_bools = [&bools] (QPUState *s, BranchCond const &cond) {
    for (int i = 0; i < NUM_LANES; i++)
      switch (cond.flag) {
        case ZS: bools[i] =  s->zeroFlags[i]; break;
        case ZC: bools[i] = !s->zeroFlags[i]; break;
        case NS: bools[i] =  s->negFlags[i];  break;
        case NC: bools[i] = !s->negFlags[i];  break;
        default: assert(false); break;
      }
  };


  switch (cond.tag) {
    case BranchCond::COND_ALWAYS: return true;
    case BranchCond::COND_NEVER: return false;
    case BranchCond::COND_ALL:
      set_bools(s, cond);
      for (int i = 0; i < NUM_LANES; i++)
        if (!bools[i]) return false;
      return true;

    case BranchCond::COND_ANY:
      set_bools(s, cond);

      for (int i = 0; i < NUM_LANES; i++)
        if (bools[i]) return true;
      return false;
    default:
      assertq(false, "checkBranchCond(): unexpected value");
      return false;
  }
}


void write_special_register(QPUState* s, State* g, bool setFlags, AssignCond cond, Reg dest, Vec v) {
  switch (dest.regId) {
    case SPECIAL_RD_SETUP: {
      int setup = v[0].intVal;
      if ((setup & 0xf0000000) == 0x90000000) {
        // Set read pitch
        int pitch = (setup & 0x1fff);
        s->readPitch = pitch;
        return;
      } else if ((setup & 0xc0000000) == 0) {
        // QPU only allows two VPM loads queued at a time
        assert(!s->vpmLoadQueue.isFull());

        // Create VPM load request
        //warn << "Create VPM load request";
        VPMLoadReq req(setup);

        // Add VPM load request to queue
        s->vpmLoadQueue.enq(req);
        return;
      } else if (setup & 0x80000000) {
        // DMA load setup
        DMALoadReq* req = &s->dmaLoadSetup;
        req->rowLen = (setup >> 20) & 0xf;
        if (req->rowLen == 0) req->rowLen = 16;
        req->numRows = (setup >> 16) & 0xf;
        if (req->numRows == 0) req->numRows = 16;
        req->vpitch = (setup >> 12) & 0xf;
        if (req->vpitch == 0) req->vpitch = 16;
        req->hor = (setup & 0x800) ? false : true;
        req->vpmAddr = (setup & 0x7ff);
        return;
      }
      break;
    }

    case SPECIAL_WR_SETUP: {
      int setup = v[0].intVal;
      if ((setup & 0xc0000000) == 0xc0000000) {
        // Set write stride
        int stride = setup & 0x1fff;
        s->writeStride = stride;
        return;
      } else if ((setup & 0xc0000000) == 0x80000000) {
        // DMA write setup
        DMAStoreReq* req = &s->dmaStoreSetup;
        req->rowLen = (setup >> 16) & 0x7f;
        if (req->rowLen == 0) req->rowLen = 128;
        req->numRows = (setup >> 23) & 0x7f;
        if (req->numRows == 0) req->numRows = 128;
        req->hor = (setup & 0x4000);
        req->vpmAddr = (setup >> 3) & 0x7ff;
        return;
      } else if ((setup & 0xc0000000) == 0) {
        VPMStoreReq req(setup);
        //warn << req.dump();
        s->vpmStoreSetup = req;
        return;
      }
      break;
    }

    case SPECIAL_VPM_WRITE: {
      vpm_write(s->vpmStoreSetup, g->vpm, v);
      return;
    }

    case SPECIAL_DMA_LD_ADDR: {
      // Initiate DMA load
      s->dmaLoad.start();
      s->dmaLoad.addr = v[0];
      return;
    }

    case SPECIAL_DMA_ST_ADDR: {
      // Initiate DMA store
      s->dmaStore.start();
      s->dmaStore.addr = v[0];
      return;
    }

    case SPECIAL_HOST_INT: {
      return;
    }

    case SPECIAL_TMUA: {
      assert(s->loadBuffer.size() < 4);
      Vec val;
      for (int i = 0; i < NUM_LANES; i++) {
        uint32_t a = (uint32_t) v[i].intVal;
        val[i].intVal = g->emuHeap.phy(a >> 2);
      }
      s->loadBuffer.append(val);
      return;
    }

    case SPECIAL_MUTEX_RELEASE:
      //warn << "read_special_register() handling SPECIAL_MUTEX_RELEASE";
      Mutex::release(s->id);
      return;

    default:
      if (s->sfu.writeReg(dest, v)) {
        return;
      }
      break;
  }

  assertq(false, "emulator: can not write to special register", true);
}


/**
 * Write a vector to a register
 */
void writeReg(QPUState* s, State* g, bool setFlags, AssignCond cond, Reg dest, Vec v) {

  switch (dest.tag) {
    case REG_A:
    case REG_B:
    case ACC:
    case NONE:
      Vec* w;

      if (dest.tag == REG_A) {
        assert(dest.regId >= 0 && dest.regId < s->sizeRegFileA);
        w = &s->regFileA[dest.regId];
      } else if (dest.tag == REG_B) {
        assert(dest.regId >= 0 && dest.regId < s->sizeRegFileB);
        w = &s->regFileB[dest.regId];
      } else if (dest.tag == ACC) {
        assert(dest.regId >= 0 && dest.regId <= 5);
        w = &s->accum[dest.regId];
      }
      
      for (int i = 0; i < NUM_LANES; i++)
        if (checkAssignCond(s, cond, i)) {
          Word x = v[i];
          if (dest.tag != NONE) w->get(i) = x;

          if (setFlags) {
            s->zeroFlags[i] = x.intVal == 0;
            s->negFlags[i]  = x.intVal < 0;
          }
        }

      return;

    case SPECIAL:
      //warn << "writeReg SPECIAL v: " << v.dump();
      write_special_register(s, g, setFlags, cond, dest, v);
      return;

    default:
      assertq(false, "emulator: unexpected dest tag");
      break;
  }
}


// ============================================================================
// Interpret a small immediate operand
// ============================================================================

Vec evalSmallImm(QPUState* s, uint32_t imm) {
  Vec v;
  Word w = decodeSmallLit(imm);

  for (int i = 0; i < NUM_LANES; i++) {
    v[i] = w;
  }

  return v;
}


Vec readRegOrImm(QPUState* s, State &state, RegOrImm const &src) {
  if (src.is_reg()) {
    Vec ret = readReg(*s, state, src.reg());
    return ret;
  } else {
    return evalSmallImm(s, src.encode());
  }
}


/**
 * @brief run the current instruction in the emulator for the selected QPU
 */
void run_instruction(QPUState &s, State &state, Instr const &instr) {
  auto ALWAYS = AssignCond::Tag::ALWAYS;

  switch (instr.tag) {
    case LI: {
      Vec imm(instr.LI.imm);
      writeReg(&s, &state, instr.set_cond().flags_set(), instr.assign_cond(), instr.dest(), imm);
    }
    break;

    case ALU:
      if (!instr.ALU.op.isNOP()) {
        Vec a;
        Vec b;

        if (instr.isUniformLoad()) {
          a = state.get_uniform(s.id, s.nextUniform);
          b = a; 
        } else {
          a = readRegOrImm(&s, state, instr.ALU.srcA);
          b = readRegOrImm(&s, state, instr.ALU.srcB);
        }

        Vec result;
        result.apply(instr.ALU.op, a, b);

        writeReg(&s, &state, instr.set_cond().flags_set(), instr.assign_cond(), instr.dest(), result);
      }
    break;

    case BR: {  // Branch to target
      if (checkBranchCond(&s, instr.branch_cond())) {
        BranchTarget t = instr.branch_target();

        if (t.relative && !t.useRegOffset) {
          s.pc += 3 + t.immOffset;
        } else {
          fatal("V3DLib: found unsupported form of branch target");
        }
      }
    }
    break;

    case RECV: {                             // receive load-via-TMU response
      assert(s.loadBuffer.size() > 0);
      Vec val = s.loadBuffer.remove(0);
      AssignCond always;
      always.tag = ALWAYS;
      writeReg(&s, &state, false, always, instr.dest(), val);
    }
    break;

    case SINC: if (state.sema_inc(instr.semaId)) s.pc--; break;
    case SDEC: if (state.sema_dec(instr.semaId)) s.pc--; break;

    case END:                                // End program (halt)
      s.running = false;
    break;

    case BRL:                                // Branch to label
      fatal("V3DLib: emulator does not support Branch to label");
    case LAB:                                // Label
    case NO_OP:
    case IRQ:
    case INIT_BEGIN:
    case INIT_END:
    break;  // ignore

    default: assert(false);
  }
}

}  // anon namespace


// ============================================================================
// Emulator
// ============================================================================

/**
 * @brief Run the kernel code on the emulator.
 *
 * This runs over the target code.
 *
 * @param numQPUs   Number of QPUs active
 * @param instrs    Instruction sequence
 * @param maxReg    Max reg id used
 * @param uniforms  Kernel parameters
 * @param heap
 */
void emulate(
  int numQPUs,
  Instr::List &instrs,
  int maxReg,
  IntList &uniforms,
  BufferObject &heap,
  bool do_debug
) {
  Platform::running_emulator(true);
  State state(numQPUs, uniforms);
  state.emuHeap.heap_view(heap);

  // Initialise state
  for (int i = 0; i < numQPUs; i++) {
    QPUState &q = state.qpu[i];
    q.id = i;
    q.init(maxReg);
  }

  bool anyRunning = true;

  Debugger debugger(state, instrs);
  if (do_debug) debugger.add_breakpoint(0);

  while (anyRunning) {
    anyRunning = false;

    // Execute an instruction in each active QPU
    for (int i = 0; i < numQPUs; i++) {
      if (Mutex::blocks(i)) continue;

      QPUState &s = state.qpu[i];

      if (s.running) {
        assert(s.pc < instrs.size());
        anyRunning = true;
        s.upkeep(state);

        Instr const instr = instrs.get(s.pc);
        run_instruction(s, state, instr);

        debugger.step(i);

        if (!s.waiting) {
          s.pc++;
        }
      }
    }
  }

  Platform::running_emulator(false);
}

}  // namespace V3DLib

