#include "SharedArray.h"

using namespace Log;

namespace V3DLib {

//////////////////////////////////////////////////
// Class BaseSharedArray
//////////////////////////////////////////////////

BaseSharedArray::BaseSharedArray(BufferObject *heap, uint32_t element_size) :
  m_heap(heap),
  m_element_size(element_size)
{}


bool BaseSharedArray::allocated() const {
  if (m_mem_size > 0) {
    assert(m_heap != nullptr);
    // assert(m_phyaddr > 0);  // Can be 0 for emu
    assert(m_usraddr != nullptr);
    assert(m_num_elems > 0);
    return true;
  } else {
    assert(m_phyaddr == 0);
    assert(m_usraddr == nullptr);
    assert(!m_is_heap_view);
    assert(m_num_elems == 0);
    return false;
  }
}


/**
 * @param n number of elements to allocate (so NOT memory size!)
 */
void BaseSharedArray::alloc(uint32_t n) {
  assert(n > 0);
  assert(m_element_size > 0);

  if (allocated()) {
    if (m_num_elems >= n) {
      // Current is large enough already
      // Note that this assumes that the element size doesn't change
      return;
    }

    cdebug << "alloc(): reallocating shared array";
    dealloc();
  }

  if (m_heap == nullptr) {
    m_heap = &getBufferObject();
  }

  // Round mem upwards to multiple of 16
  // This should be enough to assure 4-bit alignment (works! verified)
  uint32_t size = (uint32_t) (m_element_size*n);
  if ((size & 0xf) != 0) {
    //warn << "alloc(): rounding size " << size << " upward to multiple of 16";
    size = (size + 0x10) & (~0xf);
    //warn << "alloc(): rounded size: " << size << ", " << hex << size;
  }
  m_mem_size = size;

  assert(m_phyaddr == 0);
  m_phyaddr = m_heap->alloc_array(m_mem_size, m_usraddr);


  m_num_elems = n;
  assert(allocated());
}


/**
 * Forget the allocation and size and notify the underlying heap.
 */
void BaseSharedArray::dealloc() {

  if (m_mem_size > 0) {
    assert(allocated());
    assert(m_heap != nullptr);
    if (!m_is_heap_view) { 
      //warn << "Calling dealloc_array";
      m_heap->dealloc_array(m_phyaddr, m_mem_size);
      //warn << m_heap->dump();
    }

    m_phyaddr      = 0;
    m_mem_size     = 0;
    //m_element_size = 0;  // const member, can't reset
    m_num_elems    = 0;
    m_usraddr      = nullptr;
    m_is_heap_view = false;
  } else {
    // Already deallocated
    assert(!allocated());
  }
}


/**
 */
uint32_t BaseSharedArray::getAddress() const {
  // Not sure if 4-bit alignment is required for vc4, it might go well automatically
  // TODO: check this
  if (!Platform::compiling_for_vc4()) { // v3d
    assert((m_phyaddr & 0xf) == 0);
  }

  return m_phyaddr;
}


std::string BaseSharedArray::dump() const {
  std::stringstream ret;

  ret << "m_usraddr: 0x"    << std::hex << (unsigned long) m_usraddr << ", "
      << "m_phyaddr: 0x"    << std::hex << (unsigned long) m_phyaddr << ", "
      << "m_element_size: " << m_element_size;

  return ret.str();
}


void BaseSharedArray::heap_view(BufferObject &heap) {
  //warn << "BaseSharedArray::heap_view() called";
  assert(!allocated());
  assert(m_heap == nullptr);
  assert(m_element_size > 0);
  assert(m_mem_size == 0);
  assert(m_num_elems == 0);

  m_heap = &heap;
  m_is_heap_view = true;
  m_mem_size = m_heap->size();
  m_num_elems = m_mem_size/m_element_size;;
  m_usraddr = m_heap->usr_address();
  m_phyaddr = m_heap->phy_address();
}


/**
 * Get actual offset within the heap for given index of current shared array.
 *
 * @param val index of element to access
 */
uint32_t BaseSharedArray::phy(uint32_t val) {
  assert(m_phyaddr % m_element_size == 0);
  int index = (int) (val - ((uint32_t) m_phyaddr/m_element_size));
  assert(index >= 0);
  return (uint32_t) index;
}


//////////////////////////////////////////////////
// Class Code
//////////////////////////////////////////////////

/**
 * @param n - number of elements to allocate (so not number of bytes) 
 */
void Code::alloc(uint32_t n) {
  Parent::alloc(n);

  //warn << "Code alloc address: " << dump() << "; "
  //     << "allocated " << n << " elements";
}


void Code::copyFrom(std::vector<uint64_t> const &src) {
  //warn << "Code copyFrom src.size(): " << src.size() << ", "
  //     << "size(): " << size();

  Parent::copyFrom(src);
}


std::string Code::dump() const {
  return BaseSharedArray::dump();
}

}  // namespace V3DLib

