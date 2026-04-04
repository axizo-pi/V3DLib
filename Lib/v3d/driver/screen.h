#ifndef _V3D_DRIVER_SCREEN_H
#define _V3D_DRIVER_SCREEN_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"

#include "gallium/drivers/v3d/v3d_bufmgr.h"  // For init s_screen

#pragma GCC diagnostic pop


namespace s_screen {

int  get_fd();
void set_fd(int val);
bool fd_is_open();
struct v3d_screen *ptr();
void init();

}  // namespace s_screen

#endif // _V3D_DRIVER_SCREEN_H
