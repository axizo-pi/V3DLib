/**
 * Support class for interfacing with mesa library
 */
#include "BOList.h"
#include "Support/basics.h"  // Order important, must be before screen.h; otherwise compile issues on debug_break()
#include "screen.h"

using namespace Log;


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

	//v3d_set_dump_stats(false);
}


/**
 * bo should not be mapped here, that should happen when the program actually
 * wants to use a bo.
 *
 * @return handle of new bo, 0 if fail
 */
uint32_t BOList::add_handle(uint32_t size, bool warn_on_error) {
	struct v3d_bo *bo = v3d_bo_alloc(s_screen::ptr(), size, "bo_name");

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
