/**
 * Adjusted from: https://gist.github.com/notogawa/36d0cc9168ae3236902729f26064281d
 */
#ifdef QPU_MODE

#include "v3d.h"
#include "Support/basics.h"
#include "broadcom/common/v3d_device_info.h"
#include "instr/v3d_api.h"
#include <cassert>
#include <sys/ioctl.h>
#include <cstddef>    // NULL
#include <cstring>    // errno, strerror()
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>   // close(), sysconf()
#include <sstream>

namespace {

int fd = 0;

bool did_devinfo = false;
struct v3d_device_info s_devinfo = { .ver = 0 /* 0 signals no v3d */ };

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


typedef struct {
    uint32_t  handle;
    uint32_t  pad;
} gem_close;


struct st_v3d_wait_bo {
  uint32_t handle;
  uint32_t pad;
  uint64_t timeout_ns;
};


// Derived from linux/include/uapi/drm/drm.h
#define DRM_IOCTL_BASE   'd'
#define DRM_COMMAND_BASE 0x40
#define DRM_GEM_CLOSE    0x09

// Derived from linux/include/uapi/drm/v3d_drm.h
#define DRM_V3D_WAIT_BO    (DRM_COMMAND_BASE + 0x01)
#define DRM_V3D_CREATE_BO  (DRM_COMMAND_BASE + 0x02)
#define DRM_V3D_MMAP_BO    (DRM_COMMAND_BASE + 0x03)
#define DRM_V3D_GET_PARAM  (DRM_COMMAND_BASE + 0x04)
#define DRM_V3D_SUBMIT_CSD (DRM_COMMAND_BASE + 0x07)

#define IOCTL_GEM_CLOSE      _IOW(DRM_IOCTL_BASE, DRM_GEM_CLOSE, gem_close)
#define IOCTL_V3D_CREATE_BO  _IOWR(DRM_IOCTL_BASE, DRM_V3D_CREATE_BO, drm_v3d_create_bo)
#define IOCTL_V3D_MMAP_BO    _IOWR(DRM_IOCTL_BASE, DRM_V3D_MMAP_BO, drm_v3d_mmap_bo)
#define IOCTL_V3D_WAIT_BO    _IOWR(DRM_IOCTL_BASE, DRM_V3D_WAIT_BO, st_v3d_wait_bo)
#define IOCTL_V3D_SUBMIT_CSD _IOW(DRM_IOCTL_BASE, DRM_V3D_SUBMIT_CSD, st_v3d_submit_csd)

const unsigned V3D_PARAM_V3D_UIFCFG = 0;
const unsigned V3D_PARAM_V3D_HUB_IDENT1 = 1;
const unsigned V3D_PARAM_V3D_HUB_IDENT2 = 2;
const unsigned V3D_PARAM_V3D_HUB_IDENT3 = 3;
const unsigned V3D_PARAM_V3D_CORE0_IDENT0 = 4;
const unsigned V3D_PARAM_V3D_CORE0_IDENT1 = 5;
const unsigned V3D_PARAM_V3D_CORE0_IDENT2 = 6;
const unsigned V3D_PARAM_SUPPORTS_TFU = 7;
const unsigned V3D_PARAM_SUPPORTS_CSD = 8;


void log_error(int ret, char const *prefix = "") {
  if (ret == 0) return;

  char buf[256];
  sprintf(buf, "ERROR %s: %s\n", prefix, strerror(errno));
  error(buf);
}


/*
// Test done anyway for Pi5 (vc7), disabling for now till insight strikes

void warn_offset(uint32_t offset) {
	uint32_t page_size = (uint32_t) sysconf(_SC_PAGE_SIZE);

	// This apparently can be called outside of compile, can't use V3DLib::Platform::compiling_for_vc7()
	if (devinfo_ver() >= 71) {  // Doesn't work either
		// Pagesize for Pi5 (vc7) is 16KB, but everything seems to work fine despite warning
		page_size = 4*1024;
	}

  if (offset % page_size != 0) {
    std::string msg = "alloc_intern(): create_bo.offset ";
    msg << "(" << offset << ") "
        << "is not a multiple of pagesize "
        << "(" << page_size << "); "
        << "mmap() may fail";
    warning(msg);
  }
}
*/


/**
 * Allocate and map a buffer object
 *
 * This function is also used to check availability of the GPU device
 * via a given device driver (called 'card' in this code).
 * Parameter `show_perror` is used to suppress any errors if this check fails.
 *
 * Calls to ioctl will fail if `sudo` not used.
 *
 * @param show_perror  if true, suppress any errors when creating a buffer object
 *
 * ============================================================================
 * NOTES
 * =====
 *
 * * Possible reasons mmap() failing (online research):
 *
 *   - running in virtual VM - hypervisor will not allow MAP_SHARED outside of VM boundary
 *   - EINVAL from man:
 *     * We don't like addr, length, or offset (e.g., they are too large, or not aligned on a page boundary).
 *     * length was 0.
 *     * flags contained none of MAP_PRIVATE, MAP_SHARED or MAP_SHARED_VALIDATE.
 *     Remedies:
 *       * addr == NULL, as in mesa v3d_bo_map_unsynchronized(), should be OK
 *       * flags: MAP_SHARED used, is OK
 *       * offset checked for page size multiple
 *       * length? Added warning
 *   - No memory available, but would then have error ENOMEM
 *   - VM_SHARED set but open() called with no write permission set.
 *   - Total number of mapped pages exceeds `/proc/sys/vm/max_map_count`
 */
bool alloc_intern(
  int fd,
  uint32_t size,
  uint32_t &handle,
  uint32_t &phyaddr,
  void **usraddr,
  bool show_perror = true) {
  assert(fd != 0);

  drm_v3d_create_bo create_bo;
  create_bo.size = size;
  create_bo.flags = 0;
  {
    // Returns handle and offset in create_bo
    int result = ioctl(fd, IOCTL_V3D_CREATE_BO, &create_bo);
    if (show_perror) {
      log_error(result, "alloc_intern() create bo");  // `show_perror` intentionally only used here
    }

    if (result != 0) {
      handle  = 0;
      phyaddr = 0;
      return false;
    }
  }
  handle  = create_bo.handle;
  phyaddr = create_bo.offset;

  //warn_offset(create_bo.offset);

  drm_v3d_mmap_bo mmap_bo;
  mmap_bo.handle = create_bo.handle;
  mmap_bo.flags = 0;
  {
    // Returns offset to use for mmap() in mmap_bo
    int result = ioctl(fd, IOCTL_V3D_MMAP_BO, &mmap_bo);
    log_error(result, "alloc_intern() mmap bo");
    if (result != 0) return false;
  }

  {
    void *result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (__off_t) mmap_bo.offset);
    if (result == MAP_FAILED) {
      log_error(1, "alloc_intern() mmap");

      std::string msg = "mmap() failure, size: ";
      msg << size << ", offset: " << mmap_bo.offset;
      error(msg);

      return false;
    }

    *usraddr = result;
  }

  return true;
}


void fd_close(int fd) {
  if (fd > 0 ) {
#ifdef DEBUG
    assert(close(fd) >= 0);
#else
    close(fd);
#endif
  }
}


/**
 * @return  > 0 if call succeeded,
 *            0 if call failed,
 *           -1 if call failed and likely due to sudo
 */
int open_card(char const *card) {
  int fd = open(card , O_RDWR);  // This works without sudo
  if (fd == 0) {
    return 0;
  }

  //
  // Perform an operation on the device: allocate 16 bytes of memory.
  // The 'wrong' card will fail here
  //
  const uint32_t ALLOC_SIZE = 16;

  // Place a call on ithe card see if it works
  uint32_t handle = 0;
  uint32_t phyaddr = 0;
  void *usraddr = nullptr;

  bool success = alloc_intern(fd, ALLOC_SIZE, handle, phyaddr, &usraddr, false);

  // Clean up bo
  if (handle != 0) {
    assert(phyaddr != 0);
    v3d_unmap(ALLOC_SIZE, handle, usraddr);
  }

  if (!success) {
    fd_close(fd);
    fd = -1;
    //printf("open_card(): alloc test FAILED for card %s\n", card);
  }

  return fd;
}


/**
 * Call ioctl, restarting if it is interrupted
 *
 * Source: mesa/src/drm/xf86drm.c
 */
int drmIoctl(int fd, unsigned long request, void *arg)
{
    int ret;

    do {
				//(v3d_ioctl_fun)
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}


/**
 * Retrieve the device info for the current v3d device.
 *
 * @return true if call succeeded, false otherwise
 */
bool _v3d_device_info() {
	//printf("Called _v3d_device_info\n");
	if (did_devinfo) return true;  // run this exactly one
	did_devinfo = true;

	bool fd_is_open = (fd != 0);

	if (!fd_is_open) {
		// open fd if not already open
		//printf("_v3d_device_info opening fd\n");

		if (!v3d_open()) {
			error("_v3d_device_info: Could not open fd.");
			return false;
		}
	}

	bool ret =  v3d_get_device_info(fd, &s_devinfo, drmIoctl);

	if (!fd_is_open) {
		// Close fd again if not open previously
		if (!v3d_close()) {
			error("_v3d_device_info: Could not close fd.");
		}
	}

	if (!ret) {
		warning("Could not get device_info");
	}

	return ret;
}


bool v3d_wait_bo(uint32_t handle, uint64_t timeout_ns) {
  assert(handle != 0);
  assert(timeout_ns > 0);

  st_v3d_wait_bo st = {
    handle,
    0,
    timeout_ns,
  };

  int ret = ioctl(fd, IOCTL_V3D_WAIT_BO, &st);
  log_error(ret, "v3d_wait_bo()");
  assertq(ret == 0, "v3d_wait_bo(): call iotctl failed");
  return (ret == 0);
}

}  // anon namespace


/**
 *
 */
int v3d_submit_csd(st_v3d_submit_csd &st) {
  int ret = ioctl(fd, IOCTL_V3D_SUBMIT_CSD, &st);
  log_error(ret, "v3d_submit_csd()");
  assert(ret == 0);
  return ret;
}


/**
 * Apparently, you don't need to close afterwards.
 * If you try, then you get the perror:
 *
 *    Inappropriate ioctl for device
 *
 * @return true if opening succeeded, false otherwise
 */
bool v3d_open() {
  if (fd != 0) {
    return true;  // Already open, all is well
  }

  // It appears to be a random crap shoot which device card0 and card1 address
  // So we try both, test them and pick the one that works (if any)
  int fd0 = open_card("/dev/dri/card0");
  int fd1 = open_card("/dev/dri/card1");

  if (fd0 <= 0 && fd1 <= 0) {
    std::string msg = "Could not open v3d device";

    if (fd0 < 0 || fd1 < 0) {
      msg << ", did you forget 'sudo'?";
    }
    assertq(false, msg);
    return false;
  }

  fd = (fd1 <= 0)? fd0: fd1;
  assert(fd > 0);
  return true;
}


/**
 * @return true if close executed, false if already closed
 */
bool v3d_close() {
  if (fd == 0) { return false; }

  fd_close(fd);
  fd = 0;

  return true;
}


// Used in v3d/instr/v3d_api.c, hence the 'extern' brackets
extern "C" {

struct v3d_device_info const *devinfo() {
	if(!_v3d_device_info()) {
		error("devinfo: Call to _v3d_device_info() failed");
		return nullptr;
	}

	return &s_devinfo;
}

}


bool v3d_device_vc7() {
	//warning("Called v3d_device_vc7()");
	_v3d_device_info();

	// Paranoia - notify unknown version
  if (!(s_devinfo.ver == 0 || s_devinfo.ver == 42 || s_devinfo.ver == 71)) {
		warning("Unexpected device_info version");
	}

	return (s_devinfo.ver == 71);
}


std::string v3d_device_info() {
	std::stringstream buf;

	if(!_v3d_device_info()) {
		buf << "Call to _v3d_device_info() failed\n";
		return buf.str();
	}


	auto &devinfo = s_devinfo;

	buf << "v3d devinfo" << "\n"
      << "===========" << "\n"
      << "Version                   : " <<  (int) devinfo.ver                    << "\n"
	    << "Revision number           : " <<  (int) devinfo.rev                    << "\n"
			<< "Compatibility rev number  : " <<  (int) devinfo.compat_rev             << "\n"
	    << "Max # performance counters: " <<  (int) devinfo.max_perfcnt            << "\n"
	    << "VPM size                  : " <<  (int) devinfo.vpm_size  << " bytes"  << "\n"

 			// NSLC*QUPS from the core's IDENT registers. 
	    << "Num QPU's                 : " <<  (int) devinfo.qpu_count              << "\n"

	    << "Has accumulators          : " <<  (devinfo.has_accumulators?  "yes":"no")  << "\n"
	    << "Has reset counter         : " <<  (devinfo.has_reset_counter? "yes":"no")  << "\n"

			// Granularity for the Clipper XY Scaling 
	    << "Granularity               : " <<  devinfo.clipper_xy_granularity << "\n"

			// WRI: Explanatory gobbledygook:
      /** The Control List Executor (CLE) pre-fetches V3D_CLE_READAHEAD
       *  bytes from the Control List buffer. The usage of these last bytes
       *  should be avoided or the CLE would pre-fetch the data after the
       *  end of the CL buffer, reporting the kernel "MMU error from client
       *  CLE".
       */
	    << "CLE readahead             : " <<  (int) devinfo.cle_readahead << "\n"

      // Minimum size for a buffer storing the Control List Executor (CLE)
	    << "CLE buffer min size       : " <<  (int) devinfo.cle_buffer_min_size  << "\n"
	;




	return buf.str();
}


bool v3d_alloc(uint32_t size, uint32_t &handle, uint32_t &phyaddr, void **usraddr) {
  assert(size > 0);
  assert(handle == 0);
  assert(phyaddr == 0);
  assert(*usraddr == nullptr);

  return  alloc_intern(fd, size, handle, phyaddr, usraddr);
}


bool v3d_unmap(uint32_t size, uint32_t handle,  void *usraddr) {
  assert(size > 0);
  assert(handle > 0);
  assert(usraddr != nullptr);

  int res = munmap(usraddr, size);
  if (res != 0) {
    return false;
  }

  gem_close cl;
  cl.handle = handle;
  return (ioctl(fd, IOCTL_GEM_CLOSE, &cl) == 0);
}


/**
 * @return true if all waits succeeded, false otherwise
 */
bool v3d_wait_bo(std::vector<uint32_t> const &bo_handles, uint64_t timeout_ns) {
  assert(bo_handles.size() > 0);
  assert(timeout_ns > 0);

  int ret = true;

  for (auto handle : bo_handles) {
    if (!v3d_wait_bo(handle, timeout_ns)) { 
      ret = false;
    }
  }
  //printf("Done calling v3d_wait_bo()\n");

  return ret;
}

#endif  // QPU_MODE
