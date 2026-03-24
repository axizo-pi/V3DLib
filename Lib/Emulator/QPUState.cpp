#include "QPUState.h"
#include "Support/basics.h"

namespace V3DLib {


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


void QPUState::upkeep() {
  sfu.upkeep(accum[4]);
  dmaLoad.upkeep();
  dmaStore.upkeep();
}


/**
 * @brief Dump state of QPU with given index.
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
      << "running: " << ((running)?"active":"halted")
      << "\n";

  if (dmaLoad.active())  ret << "dmaLoad: " << dmaLoad.dump() << "\n";
  if (dmaStore.active()) ret << "dmaStore: " << dmaStore.dump() << "\n";

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

} // namespace V3DLib

