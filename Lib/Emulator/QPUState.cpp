#include "QPUState.h"
#include "Support/basics.h"
#include "DMA.h"

namespace V3DLib {

using namespace Log;

/**
 * @return true if input handled, false otherwise
 */
bool SFU::writeReg(Reg dest, Vec v) {
  if (dest.tag != SPECIAL) return false;
  bool handled = true;

  switch (dest.regId) {
    case SPECIAL_SFU_RECIP:     value = v.recip();      break;
    case SPECIAL_SFU_RECIPSQRT: value = v.recip_sqrt(); break;
    case SPECIAL_SFU_EXP:       value = v.exp();        break;
    case SPECIAL_SFU_LOG:       value = v.log();        break;
    default:
      handled = false;
      break;
  }

  if (handled) {
    assertq(timer == -1, "SFU is running on SFU function call", true);
    timer = 3;
  }

  return handled;
}


void SFU::upkeep(Vec &r4) {
  if (timer > 0) {
    timer--;
  }

  if (timer == 0) {
    // finish pending SFU function
    for (int i = 0; i < NUM_LANES; i++) {
      r4[i].floatVal = value[i].floatVal;
    }
    timer = -1;
  }
}  


/////////////////////////////////////////////////////
// Class QPUState
/////////////////////////////////////////////////////

QPUState::QPUState() {
  for (int i = 0; i < NUM_LANES; i++) negFlags[i] = false;
  for (int i = 0; i < NUM_LANES; i++) zeroFlags[i] = false;
}


QPUState::~QPUState() {
  delete [] regFileA;
  delete [] regFileB;
}


void QPUState::init(int maxReg) {
  running      = true;
  regFileA     = new Vec [maxReg+1];
  sizeRegFileA = maxReg+1;
  regFileB     = new Vec [maxReg+1];
  sizeRegFileB = maxReg+1;
}


void QPUState::upkeep(State &state) {
  sfu.upkeep(accum[4]);

  dmaLoad.upkeep();
  if (dmaLoad.done_waiting()) {
    cdebug << "QPUState::upkeep() DMA load done waiting";
    DMA::load(this, &state);
    dmaLoad.active(false);
  }

  dmaStore.upkeep();
  if (dmaStore.done_waiting()) {
    cdebug << "QPUState::upkeep() DMA store done waiting";
    DMA::store(this, &state);
    dmaStore.active(false);
  }

	if (m_take_jump) {
		m_jump_count++;

		if (m_jump_count >= MAX_JUMP_COUNT) {  // >= paranoia
			m_take_jump = false;
			pc += m_jump_offset;
			m_jump_count  = 0;
			m_jump_offset = 0;
		}
	}

  waiting = false;
}


/**
 * @brief Dump state of QPU with given index.
 *
 * @param index index of QPU that is dumped; -1 if not passed.
 *
 * This is not intended to be complete; will fill it in as I go.
 */
MAYBE_UNUSED std::string QPUState::dump(int index) const {
  std::string ret;

  auto disp_bool_array = [] (bool const *arr) -> std::string {
    std::string ret;

    for (int i = 0; i < NUM_LANES; ++i) {
      ret << (arr[i]?"1":"0");
    }
    return ret;
  };

  ret << "\nPartial state QPU ";
  if (index != -1) {
    ret << index;
  }

  ret << "\n"
      << "--------------------\n"
      << "PC: " << pc << "\n"
      << "running: " << dump_runstate() << "\n";

	if (m_take_jump) {
  	ret << "jump count: " << m_jump_count << ", offset: " << m_jump_offset  << "\n";
	}

  ret << "\n";

  if (dmaLoad.active())  ret << "dmaLoad: " << dmaLoad.dump() << "\n";
  if (dmaStore.active()) ret << "dmaStore: " << dmaStore.dump() << "\n";

	auto const &v = vpmLoadQueue;
	if (!v.isEmpty()) {
		ret << "vpmLoadQueue: ";
		for (int cur = v.front; cur != v.back; cur = (cur + 1)% v.size) {
	 		ret<< v.elems[cur].dump() << ", ";
		}
		ret << "\n";
	}

  ret << "negFlags : " << disp_bool_array(negFlags)  << "\n"
      << "zeroFlags: " << disp_bool_array(zeroFlags) << "\n";

  // Show content accumulators
  ret << "Accumulators:\n";
  for (int i = 0; i < 6; ++i) {
    ret << "  ACC" << i << ": " << accum[i].dump() << "\n";
  }

  int count = 6;
  ret << "Regfile A, first " << count << " values:\n";
  for (int i = 0; i < count; ++i) {
    ret << "  " << i << ": " << regFileA[i].dump() << "\n";
  }

  ret << "Regfile B, first " << count << " values:\n";
  for (int i = 0; i < count; ++i) {
    ret << "  " << i << ": " << regFileB[i].dump() << "\n";
  }

  return ret;
}


std::string QPUState::dump_runstate() const {
	return (running?(waiting?"waiting/stalled":"active"):"halted");
}

} // namespace V3DLib

