/**
 * Adjusted from: https://gist.github.com/notogawa/36d0cc9168ae3236902729f26064281d
 */
#ifdef QPU_MODE

#define USE_MESA_BUFMGR 1

#include "v3d.h"
#include "Support/basics.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include "global/log.h"
#include <fcntl.h>
#include "broadcom/common/v3d_device_info.h"
#include <cstring>    // errno, strerror()
#include <sys/mman.h>
#include "util/ralloc.h"
#include <unistd.h>   // close(), sysconf()

using namespace Log;

namespace {

void fd_close(int fd) {
  if (fd > 0 ) {
    assert(close(fd) >= 0);
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


//////////////////////////////////////
// Support for alloc_intern
//////////////////////////////////////

/**
 * Allocate and map a buffer object
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
  bool show_perror = true) {
  assert(fd != 0);

  drm_v3d_create_bo create_bo;
  create_bo.size = size;
  create_bo.flags = 0;
  {
    // Returns handle and offset in create_bo
    int result = ioctl(fd, IOCTL_V3D_CREATE_BO, &create_bo);
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


bool v3d_wait_bo(uint32_t handle, uint64_t timeout_ns) {
  assert(handle != 0);
  assert(timeout_ns > 0);

  drm_v3d_wait_bo st = {
    handle,
    0,
    timeout_ns,
  };

  int ret = ioctl(fd, DRM_IOCTL_V3D_WAIT_BO, &st);
	if (ret != 0) {
  	cerr << "v3d_wait_bo(): " << strerror(errno); 
	}
  assertq(ret == 0, "v3d_wait_bo(): call iotctl failed");
  return (ret == 0);
}

}  // anon namespace



bool v3d_alloc(uint32_t size, uint32_t &handle, uint32_t &phyaddr, void **usraddr) {
  assert(size > 0);
  assert(handle == 0);
  assert(phyaddr == 0);
  assert(*usraddr == nullptr);

  return alloc_intern(fd, size, handle, phyaddr, usraddr);
}


bool v3d_unmap(uint32_t size, uint32_t handle,  void *usraddr) {
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
void set_fd(int val) { fd = val; }
bool fd_is_open() { return get_fd() > 0; }


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

  // Place a call on the card see if it works
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
		set_fd(0);
    fd = -1;
    //printf("open_card(): alloc test FAILED for card %s\n", card);
  }

  return fd;
}


#else  //  USE_MESA_BUFMGR == 1

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"


#include "gallium/drivers/v3d/v3d_bufmgr.h"  // For init s_screen


#pragma GCC diagnostic pop

namespace {

struct v3d_screen screen_init = {
        //struct pipe_screen base;
        //.renderonly = nullptr,  //  error: ‘v3d_screen’ has no non-static data member named ‘renderonly’
        .fd = 0,
        .name = "s_screen",
        .perfcnt = nullptr,
        //struct slab_parent_pool transfer_pool;

        .bo_cache = {
                .size_list = nullptr,
                .size_list_size = 0
                //mtx_t lock
				},

        .compiler = nullptr,
        .bo_handles = nullptr,
        //mtx_t bo_handles_mutex;
        .bo_size = 0,
        .bo_count = 0,
        //uint32_t prim_types;

        .has_csd = false,
        .has_cache_flush = false,
        .has_perfmon = false,
        .nonmsaa_texture_size_limit = false,
        .has_cpu_queue = false,
        .has_multisync = false,
};


struct v3d_screen *s_screen = nullptr; 


void s_screen_init() {
	s_screen = (struct v3d_screen *) ralloc_size(nullptr, sizeof(struct v3d_screen));
	assert(s_screen != nullptr);	

	memcpy( (void *) s_screen, &screen_init, sizeof(struct v3d_screen));

	list_inithead(&(s_screen->bo_cache.time_list));

	// NOTE: Not cleaned up
	struct list_head *head = (struct list_head *) ralloc_size(nullptr, sizeof(struct list_head));
	list_inithead(head);
  s_screen->bo_cache.size_list = head;
}


bool s_screen_initialized() {
  return (s_screen != nullptr);
  // return s_screen.bo_cache.size_list != nullptr;
}


}  // anon namespace


int  get_fd() {
	if (s_screen == nullptr) return 0;
	return s_screen->fd;
}


void set_fd(int val) {
	assert(s_screen != nullptr);	
	s_screen->fd = val;
}


bool fd_is_open() { return get_fd() > 0; }


namespace {

class BOList : public std::vector<struct v3d_bo *> {
public:
	~BOList();

	uint32_t       add_handle(uint32_t size, bool warn_on_error = false);
	struct v3d_bo *by_handle(uint32_t handle) const;
	bool           delete_by_handle(uint32_t handle);

private:
	void unreference(struct v3d_bo *ptr);
};


void BOList::unreference(struct v3d_bo *ptr) {
	v3d_bo_unreference(&ptr);
	FREE(ptr);
}


/**
 * Confirmed: called on program exit
 */
BOList::~BOList() {
	//warn << "~BOList() called";
	//v3d_set_dump_stats(true);

	// Delete all present items
	for (auto ptr: *this) {
		unreference(ptr);
	}

	clear();

	if (s_screen_initialized()) {
		// This method is one of the last things called on exit, reasonable place to clean up screen 
		// Following call takes care of unmap. See v3d_bo_free() in v3d_bufmgr.c
		v3d_bufmgr_destroy_w(s_screen);
	}

	//v3d_set_dump_stats(false);
}


/**
 * bo should not be mapped here, that should happen when the program actually
 * wants to use a bo.
 *
 * @return handle of new bo, 0 if fail
 */
uint32_t BOList::add_handle(uint32_t size, bool warn_on_error) {
	assert(s_screen_initialized());

	struct v3d_bo *bo = v3d_bo_alloc(s_screen, size, "bo_name");

	if (bo == nullptr) {
		if (warn_on_error) {
			warn << "BOList::add_handle: alloc failed";
		}
		return 0;
	}

	// Pedantic: the returned bo might come from the cache in mesa bufmgr.
  // It might have been mapped already.
	//
	// mesa bufmgr unmaps when it sees fit.
	// Warn me if this happens.
	if (bo->map != nullptr) cdebug << "BOList::add_handle: already mapped ";

	push_back(bo);
	return bo->handle;
}


struct v3d_bo *BOList::by_handle(uint32_t handle) const {

	for (auto ptr: *this) {
		if (ptr->handle == handle) return ptr;
	}

	assertq(false, "by_handle: ptr not found");

	return nullptr;
}


bool BOList::delete_by_handle(uint32_t handle) {

	// Find index of given handle
	int index = -1;
	for (int i = 0; i < (int) size(); ++i) {
		if (at(i)->handle == handle) {
			index = i;
			break;
		}
	}

	if (index == -1) return false;

	auto ptr = at(index);
	unreference(ptr);
	erase(begin() + index);
	return true;
}

BOList s_bolist;


/**
 * @return  > 0 if call succeeded,
 *            0 if call failed,
 *           -1 if call failed and likely due to sudo
 */
int open_card(char const *card) {
	s_screen_init();  // First thing that is called, good place to init

  int fd = open(card , O_RDWR);  // This works without sudo
  if (fd == 0) return 0;
	set_fd(fd);  // NOTE: global fd set here, required by logic but not kosher

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
    //cerr << "open_card(): alloc test FAILED for card " << card;
  }

  return fd;
}

}  // anon namespace


bool v3d_wait_bo(uint32_t handle, uint64_t timeout_ns) {
  assert(handle != 0);
  assert(timeout_ns > 0);

	struct v3d_bo *bo = s_bolist.by_handle(handle);
	assert(bo != nullptr);

	const char *reason = nullptr;
	bool ret = v3d_bo_wait(bo, timeout_ns, reason);

	if (!ret) {
		cerr << "v3d_bo_wait failed. Reason: " << reason;
	}

	return ret;
}


bool v3d_alloc(uint32_t size, uint32_t &out_handle, uint32_t &phyaddr, void **usraddr) {
	bool ret = v3d_open();  // ensure open
	assert(ret);
	assert(fd_is_open());

  assert(size > 0);
  assert(out_handle == 0);
  assert(phyaddr == 0);
  assert(*usraddr == nullptr);
	//v3d_set_dump_stats(true);

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

	//v3d_set_dump_stats(false);
	return true;
}


/**
 * No need to explicitly unmap, mesa lib deals with it.
 * It's kind of complicated to do it explicitly due to internal bo caching in mesa v3d_bufmgr.c
 * Not dealing with this.
 */ 
bool v3d_unmap(uint32_t size, uint32_t handle, void *usraddr) {
	assert(usraddr != nullptr);

	// bo may already have been removed, let it happen
	s_bolist.delete_by_handle(handle);

	return true;
}

#endif // USE_MESA_BUFMGR


////////////////////////////////////////////////////////
// Common calls
////////////////////////////////////////////////////////

int v3d_submit_csd(drm_v3d_submit_csd &st) {
  int ret = ioctl(get_fd(), DRM_IOCTL_V3D_SUBMIT_CSD, &st);
	if (ret != 0) {
  	cerr << "v3d_submit_csd(): " << strerror(errno); 
	}
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
  if (fd_is_open()) return true;  // Already open, all is well

  // It appears to be a random crap shoot which device card0 and card1 address
  // So we try both, test them and pick the one that works (if any)
  int fd0 = open_card("/dev/dri/card0");
  int fd1 = open_card("/dev/dri/card1");

  if (fd0 <= 0 && fd1 <= 0) {
    std::string msg = "Could not open v3d device, did you forget 'sudo'?";
    assertq(false, msg);
    return false;
  }

  int fd = (fd1 <= 0)? fd0: fd1;
  assert(fd > 0);
	set_fd(fd);
  return true;
}


/**
 * Not called anywhere
 * TODO: Check if this is OK
 *
 * @return true if close executed, false if already closed
 */
bool v3d_close() {
	breakpoint;
	int fd = get_fd();
  if (fd == 0) { return false; }

  fd_close(fd);
  set_fd(0);

  return true;
}


/**
 * @return true if all waits succeeded, false otherwise
 */
bool v3d_wait_bo(BoHandles const &bo_handles, uint64_t timeout_ns) {
  assert(bo_handles.size() > 0);
  assert(timeout_ns > 0);

  int ret = true;

  for (auto handle : bo_handles) {
    if (!v3d_wait_bo(handle, timeout_ns)) { 
      ret = false;
    }
  }

  return ret;
}

#endif  // QPU_MODE
