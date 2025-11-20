#ifndef _V3D_V3D_h
#define _V3D_V3D_h

#ifdef QPU_MODE

#include <stdint.h>
#include <vector>
#include <string>

using BoHandles  = std::vector<uint32_t>;

struct st_v3d_submit_csd {
  uint32_t cfg[7];
  uint32_t coef[4];
  uint64_t bo_handles;
  uint32_t bo_handle_count;
  uint32_t in_sync;
  uint32_t out_sync;
};


bool v3d_open();
bool v3d_close();
int v3d_submit_csd(st_v3d_submit_csd &st);
bool v3d_wait_bo(BoHandles const &bo_handles, uint64_t timeout_ns);
bool v3d_alloc(uint32_t size, uint32_t &handle, uint32_t &phyaddr, void **usraddr);
bool v3d_unmap(uint32_t size, uint32_t handle, void *usraddr);

bool v3d_device_vc7();
std::string v3d_device_info();

#endif  // QPU_MODE

#endif  // _V3D_V3D_h
