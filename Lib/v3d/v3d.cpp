/**
 * Adjusted from: https://gist.github.com/notogawa/36d0cc9168ae3236902729f26064281d
 */
#include "global/log.h"

using namespace Log;

#ifdef QPU_MODE

#define USE_MESA_BUFMGR 1

#include "v3d.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <cstring>    // errno, strerror()
#include <sys/mman.h>
#include <unistd.h>   // close(), sysconf()
#include "string.h"  // strerror
#include "Support/Platform.h"

using namespace V3DLib;

namespace {

void fd_close(int fd) {
  if (fd > 0 ) {
    assert(close(fd) >= 0, "fd_close() failed");
    close(fd);
  }
}

}  // anon namespace

#if USE_MESA_BUFMGR == 0

#include "instr/v3d_api.h"
#include <cstddef>    // NULL
#include <cassert>

namespace {

int fd = 0;


/**
 * @brief Allocate and map a buffer object
 *
 * NOTE: This is an old-style alloc call and ideally would not be used 
 *       with the mesa alloc. However, it is used for testing a device in open()
 *       and therefore needs to be available
 *
 * This function is used to check availability of the GPU device
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
  bool show_perror = true
) {
  assert(fd != 0);

  drm_v3d_create_bo create_bo;
  create_bo.size = size;
  create_bo.flags = 0;
  {
    // Returns handle and offset in create_bo
    int result = ioctl(fd, DRM_IOCTL_V3D_CREATE_BO, &create_bo);
    if (show_perror && result != 0) {                              // `show_perror` intentionally only used here
      cerr <<  "alloc_intern() create bo: " << strerror(errno); 
    }

    if (result != 0) {
      handle  = 0;
      phyaddr = 0;
      return false;
    }
  }
  handle  = create_bo.handle;
  phyaddr = create_bo.offset;

  drm_v3d_mmap_bo mmap_bo;
  mmap_bo.handle = create_bo.handle;
  mmap_bo.flags = 0;
  {
    // Returns offset to use for mmap() in mmap_bo
    int result = ioctl(fd, DRM_IOCTL_V3D_MMAP_BO, &mmap_bo);
    if (result != 0) {
       cerr << "alloc_intern() mmap bo: " << strerror(errno);
      return false;
    }
  }

  {
    void *result = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (__off_t) mmap_bo.offset);
    if (result == MAP_FAILED) {
       cerr << "alloc_intern() mmap: " << strerror(errno); 
      cerr << "mmap() failure, size: " << size << ", offset: " << (int) mmap_bo.offset;
      return false;
    }

    *usraddr = result;
  }

  return true;
}

}  // anon namespace


namespace v3d {


bool alloc(uint32_t size, uint32_t &handle, uint32_t &phyaddr, void **usraddr) {
  assert(size > 0);
  assert(handle == 0);
  assert(phyaddr == 0);
  assert(*usraddr == nullptr);

  open();

  return alloc_intern(fd, size, handle, phyaddr, usraddr);
}


bool unmap(uint32_t size, uint32_t handle,  void *usraddr) {
  assert(size > 0);
  assert(handle > 0);
  assert(usraddr != nullptr);

  int res = munmap(usraddr, size);
  if (res != 0) {
    return false;
  }

  drm_gem_close cl;
  cl.handle = handle;
  return (ioctl(fd, DRM_IOCTL_GEM_CLOSE, &cl) == 0);
}


int  get_fd() { return fd; }


bool wait_bo(uint32_t handle, uint64_t timeout_ns) {
  assert(handle != 0);
  assert(timeout_ns > 0);

  drm_v3d_wait_bo st = {
    handle,
    0,
    timeout_ns,
  };

  int ret = ioctl(fd, DRM_IOCTL_V3D_WAIT_BO, &st);
  if (ret != 0) {
    cerr << "wait_bo(): " << strerror(errno); 
  }
  assertq(ret == 0, "wait_bo(): call iotctl failed");
  return (ret == 0);
}

} // namespace v3d


void set_fd(int val) { fd = val; }
bool fd_is_open() { return ::v3d::get_fd() > 0; }


/**
 * @return  > 0 if call succeeded,
 *            0 if call failed,
 *           -1 if call failed and likely due to sudo
 */
int open_card(char const *card) {
  //warn << "open_card() called, card: " << card;

  int fd = open(card , O_RDWR);  // This works without sudo
  if (fd == 0) {
    return 0;
  }

  //
  // Perform an operation on the device: allocate 16 bytes of memory.
  // The 'wrong' card will fail here
  //
  const uint32_t ALLOC_SIZE = 16;

  // Place a call on the card see if it works
  uint32_t handle = 0;
  uint32_t phyaddr = 0;
  void *usraddr = nullptr;

  bool success = alloc_intern(fd, ALLOC_SIZE, handle, phyaddr, &usraddr, false);

  // Clean up bo
  if (handle != 0) {
    assert(phyaddr != 0);
    v3d::unmap(ALLOC_SIZE, handle, usraddr);
  }

  if (!success) {
    fd_close(fd);
    set_fd(0);
    fd = -1;
    //printf("open_card(): alloc test FAILED for card %s\n", card);
  }

  return fd;
}

#else  //  USE_MESA_BUFMGR == 1

#include "driver/screen.h"
#include "driver/BOList.h"


void set_fd(int val) {
  return s_screen::set_fd(val);
}


bool fd_is_open() {
  return s_screen::fd_is_open();
}

namespace {

BOList s_bolist;


/**
 * @return  > 0 if call succeeded,
 *            0 if call failed,
 *           -1 if call failed and likely due to sudo
 */
int open_card(char const *card) {
  s_screen::init();  // First thing that is called, good place to init

  int fd = open(card , O_RDWR);  // This works without sudo
  if (fd == 0) return 0;
  set_fd(fd);

  //
  // Perform an operation on the device: allocate 16 bytes of memory.
  // The 'wrong' card will fail here
  //
  const uint32_t ALLOC_SIZE = 16;

  // Place a call on the card see if it works
  uint32_t handle = s_bolist.add_handle(ALLOC_SIZE, false);

  // Clean up bo
  if (handle != 0) {
    bool ret = s_bolist.delete_by_handle(handle);
    assert(ret);
  } else {
    fd_close(fd);
    set_fd(0);
    fd = -1;
  }

  return fd;
}

}  // anon namespace


namespace v3d {

bool wait_bo(uint32_t handle, uint64_t timeout_ns) {
  assert(handle != 0);
  assert(timeout_ns > 0);

  struct v3d_bo *bo = s_bolist.by_handle(handle);
  assert(bo != nullptr);

  bool ret = v3d_bo_wait(bo, timeout_ns, nullptr);

  if (!ret) {
    cerr << "v3d_bo_wait failed.";
  }

  return ret;
}

int get_fd() {
  //assert(s_screen::get_fd() != 0);
  return s_screen::get_fd();
}

bool alloc(uint32_t size, uint32_t &out_handle, uint32_t &phyaddr, void **usraddr) {
  bool ret = v3d::open();  // ensure open
  assert(ret);
  assert(fd_is_open());

  assert(size > 0);
  assert(out_handle == 0);
  assert(phyaddr == 0);
  assert(*usraddr == nullptr);

  uint32_t handle = s_bolist.add_handle(size); 
  assert(handle > 0);

  auto bo = s_bolist.by_handle(handle);
  assert(bo != nullptr);

  // Call map to get the user address
  auto map = v3d_bo_map(bo);  // sets member map in bo
  assert(map != nullptr);

  out_handle = handle;
  phyaddr    = bo->offset;
  *usraddr   = bo->map;

  return true;
}


/**
 * No need to explicitly unmap, mesa lib deals with it.
 * It's kind of complicated to do it explicitly due to internal bo caching in mesa v3d_bufmgr.c
 * Not dealing with this.
 */ 
bool unmap(uint32_t size, uint32_t handle, void *usraddr) {
  assert(usraddr != nullptr);

  // bo may already have been removed, let it happen
  s_bolist.delete_by_handle(handle);

  return true;
}

} // namespace v3d

#endif // USE_MESA_BUFMGR


////////////////////////////////////////////////////////
// Common calls
////////////////////////////////////////////////////////

namespace v3d {

int submit_csd(drm_v3d_submit_csd &st) {
  int ret = ioctl(get_fd(), DRM_IOCTL_V3D_SUBMIT_CSD, &st);
  if (ret != 0) {
    cerr << "v3d::submit_csd(): " << strerror(errno); 
  }
  return ret;
}


/**
 * Not called anywhere
 * TODO: Check if this is OK
 *
 * @return true if close executed, false if already closed
 * /
bool close() {
  breakpoint;
  int fd = get_fd();
  if (fd == 0) { return false; }

  fd_close(fd);
  set_fd(0);

  return true;
}
*/


int ioctl(unsigned cmd, void *param) {
  int fd = get_fd();
  assert(fd != 0);
  warn << "ioctl using fd: " << fd;

  int ret = ::ioctl(fd, cmd, param);
  if (ret != 0) {
    warn << "ioctl failed, error: " << strerror(ret);
  }

  return ret;
}

} // namespace v3d

#endif  // QPU_MODE


namespace v3d {

/**
 * Apparently, you don't need to close afterwards.
 * If you try, then you get the perror:
 *
 *    Inappropriate ioctl for device
 *
 * @return true if opening succeeded, false otherwise
 */
bool open() {
#ifdef QPU_MODE

  if (Platform::compiling_for_vc4()) {
    cerr << "Running on vc4, not opening the v3d card.";
    return true;
  }

  if (fd_is_open()) return true;  // Already open, all is well

  // It appears to be a random crap shoot which device card0 and card1 address
  // So we try both, test them and pick the one that works (if any)
  int fd0 = open_card("/dev/dri/card0");
  int fd1 = open_card("/dev/dri/card1");

  if (fd0 <= 0 && fd1 <= 0) {
    std::string msg = "Could not open v3d device, did you forget 'sudo'?";
    Log::assertq(false, msg);
    return false;
  }

  int fd = (fd1 <= 0)? fd0: fd1;
  cdebug << "Got fd: " << fd;
  assert(fd > 0);

  set_fd(fd);

#else  
  info << "Running in non-QPU mode, not opening the v3d card.";
#endif  // QPU_MODE
  return true;

}

} // namespace v3d
