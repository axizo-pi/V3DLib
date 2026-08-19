#ifndef _V3DLIB_VC4_BUFFEROBJECT_H_
#define _V3DLIB_VC4_BUFFEROBJECT_H_
#include "Common/BufferObject.h"

namespace V3DLib {
namespace vc4 {

/**
 * @brief Allocate vc4 BO.
 * 
 * An issue here is that allocated BO's do not get returned when a fatal error occurs.
 * 
 * The consequence is that eventually, new calls to V3DLib apps fail because
 * BO's can not be allocated any more.
 * 
 * Currently not sure if this can be fixed. Perhaps I can replace any `fatal()`
 * or similar calls with expections and hope that the BO's get cleaned up on
 * shutdown (TODO).
 * 
 * `v3d` does not have this issue.
*/
class BufferObject : public V3DLib::BufferObject {
public:
  ~BufferObject();

  static BufferObject &getHeap();

private:
  uint32_t handle = 0;

  void alloc_mem(uint32_t size_in_bytes) override;
  void dealloc();
};

}  // namespace vc4
}  // namespace V3DLib

#endif  // _V3DLIB_VC4_BUFFEROBJECT_H_
