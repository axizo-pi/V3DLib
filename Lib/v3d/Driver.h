#ifndef _V3DLIB_V3D_DRIVER_H_
#define _V3DLIB_V3D_DRIVER_H_
#include "Common/SharedArray.h"

namespace V3DLib {
namespace v3d {

class Driver {
  using BoHandles  = std::vector<uint32_t>;

public:
  void add_bo(uint32_t handle) { m_bo_handles.push_back(handle); }
  int num_handles() { return (int) m_bo_handles.size(); }

  bool execute(Code &code, Data *uniforms = nullptr, uint32_t thread = 1, bool wait_complete = true);
  bool wait_bo();

private:
  BoHandles m_bo_handles;
};

}  // v3d
}  // V3DLib

#endif // _V3DLIB_V3D_DRIVER_H_
