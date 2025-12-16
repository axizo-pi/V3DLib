#include "BufferObject.h"
#include <cassert>
#include <memory>
#include <cstdio>
#include "../Support/basics.h"
#include "../Support/debug.h"
#include "LibSettings.h"

namespace V3DLib {
namespace emu {

namespace {

// Defined as a smart ptr to avoid issues on program init
std::unique_ptr<BufferObject> emuHeap;

}


/**
 * Allocate memory for buffer object if not already done so
 */
void BufferObject::alloc_mem(uint32_t size_in_bytes) {
  assert(arm_base  == nullptr);

  //
  // phyaddr can not be zero, even though it is perfectly legal for this BufferObject,
  // which resides in main memory.  Zero screws up logic elsewhere.
  //
  // (Multiple of) 16 to mimic alignment of the vc4/v3d phyaddr values.
  //
  // I am quite amazed that this ever worked with zero. Did I switch universes?
  // Unit tests were passing.
  //
  uint32_t OFFSET = 16;

  arm_base = new uint8_t [OFFSET + size_in_bytes];
  set_size(size_in_bytes);
  set_phy_address(OFFSET);  // Not 0, other code start complaining
}


uint32_t BufferObject::alloc_array(uint32_t size_in_bytes, uint8_t *&array_start_address) {
  assert(arm_base != nullptr);
  return Parent::alloc_array(size_in_bytes, array_start_address);
}


BufferObject &BufferObject::getHeap() {
  if (!emuHeap) {
    //debug("Allocating emu heap v3d\n");
    emuHeap.reset(new BufferObject());
    emuHeap->alloc(LibSettings::heap_size());
  }

  return *emuHeap;
}


}  // namespace emu
}  // namespace V3DLib
