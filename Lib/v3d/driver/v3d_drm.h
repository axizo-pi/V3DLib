#ifndef _V3DLIB_V3D_DRM_H
#define _V3DLIB_V3D_DRM_H

/**
 * Isolate drm definitions in preparation of using /usr/include/v3d_drm.h
 *
 * v3d_drm.h does not exist in Debian 10 Buster, present in Debian 12 Bookworm
 * Eventually, we will convert all Pi's to Debian 12.
 *
 * NOTE: GEM stuff not officially part ot v3d_drm
 */


typedef struct {
  uint32_t cfg[7];
  uint32_t coef[4];
  uint64_t bo_handles;
  uint32_t bo_handle_count;
  uint32_t in_sync;
  uint32_t out_sync;
  uint32_t flags;
  uint32_t perfmon_id;
} drm_v3d_submit_csd;


typedef struct {
    uint32_t  handle;
    uint32_t  pad;
} drm_gem_close;


typedef struct {
  uint32_t handle;
  uint32_t pad;
  uint64_t timeout_ns;
} drm_v3d_wait_bo;


typedef struct {
    uint32_t size   = 0;
    uint32_t flags  = 0;
    uint32_t handle = 0;
    uint32_t offset = 0;
} drm_v3d_create_bo;


typedef struct {
    uint32_t handle = 0;
    uint32_t flags  = 0;
    uint64_t offset = 0;
} drm_v3d_mmap_bo;


#define DRM_IOCTL_BASE   'd'
#define DRM_COMMAND_BASE 0x40

#define DRM_V3D_WAIT_BO    (DRM_COMMAND_BASE + 0x01)
#define DRM_V3D_CREATE_BO  (DRM_COMMAND_BASE + 0x02)
#define DRM_V3D_MMAP_BO    (DRM_COMMAND_BASE + 0x03)
#define DRM_V3D_GET_PARAM  (DRM_COMMAND_BASE + 0x04)
#define DRM_V3D_SUBMIT_CSD (DRM_COMMAND_BASE + 0x07)

#define DRM_IOCTL_V3D_WAIT_BO    _IOWR(DRM_IOCTL_BASE, DRM_V3D_WAIT_BO, drm_v3d_wait_bo)
#define DRM_IOCTL_V3D_CREATE_BO  _IOWR(DRM_IOCTL_BASE, DRM_V3D_CREATE_BO, drm_v3d_create_bo)
#define DRM_IOCTL_V3D_MMAP_BO    _IOWR(DRM_IOCTL_BASE, DRM_V3D_MMAP_BO, drm_v3d_mmap_bo)
#define DRM_IOCTL_V3D_SUBMIT_CSD _IOW(DRM_IOCTL_BASE, DRM_V3D_SUBMIT_CSD, drm_v3d_submit_csd)

#define DRM_GEM_CLOSE    0x09

#define DRM_IOCTL_GEM_CLOSE      _IOW(DRM_IOCTL_BASE, DRM_GEM_CLOSE, drm_gem_close)


// Following not in v3d_drm.h - I don't think they are used
const unsigned V3D_PARAM_V3D_UIFCFG = 0;
const unsigned V3D_PARAM_V3D_HUB_IDENT1 = 1;
const unsigned V3D_PARAM_V3D_HUB_IDENT2 = 2;
const unsigned V3D_PARAM_V3D_HUB_IDENT3 = 3;
const unsigned V3D_PARAM_V3D_CORE0_IDENT0 = 4;
const unsigned V3D_PARAM_V3D_CORE0_IDENT1 = 5;
const unsigned V3D_PARAM_V3D_CORE0_IDENT2 = 6;
const unsigned V3D_PARAM_SUPPORTS_TFU = 7;
const unsigned V3D_PARAM_SUPPORTS_CSD = 8;

#endif // _V3DLIB_V3D_DRM_H
