#ifndef _V3D_V3D_H
#define _V3D_V3D_H

#ifdef QPU_MODE

#include <stdint.h>
#include <vector>
#include <string>
//#include <drm/v3d_drm.h> // What we will use eventually on Debian 12
#include "driver/v3d_drm.h"

using BoHandles  = std::vector<uint32_t>;

bool v3d_open();
bool v3d_close();
int v3d_submit_csd(drm_v3d_submit_csd &st);
bool v3d_wait_bo(BoHandles const &bo_handles, uint64_t timeout_ns);
bool v3d_alloc(uint32_t size, uint32_t &handle, uint32_t &phyaddr, void **usraddr);
bool v3d_unmap(uint32_t size, uint32_t handle, void *usraddr);

#endif  // QPU_MODE

#endif  // _V3D_V3D_H
