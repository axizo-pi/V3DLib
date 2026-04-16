#ifndef _V3D_V3D_H
#define _V3D_V3D_H

#ifdef QPU_MODE

#include "driver/v3d_drm.h"  
//#include <drm/v3d_drm.h> // What we will use eventually on Debian 12

namespace v3d {

int submit_csd(drm_v3d_submit_csd &st);
bool wait_bo(uint32_t handle, uint64_t timeout_ns);
bool alloc(uint32_t size, uint32_t &handle, uint32_t &phyaddr, void **usraddr);
bool unmap(uint32_t size, uint32_t handle, void *usraddr);
int get_fd();
int ioctl(unsigned cmd, void *param);

}

#endif  // QPU_MODE


namespace v3d {

bool open();

}

#endif  // _V3D_V3D_H
