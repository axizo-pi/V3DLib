/**
 * The screen struct is only used to interface with the bugmgr in the mesa library.
 */
#include "screen.h"
#include "global/log.h"
#include "util/ralloc.h"

using namespace Log;

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


/**
 * A goal of this class is to clean up the  screen ptr on shutdown.
 */
class Screen {
public:	
	~Screen();

	bool initialized() const { return (p_screen != nullptr); }

	struct v3d_screen *p_screen = nullptr; 
};


/**
 * If an instance is defined as a singleton, this is one of the last things called on exit.
 */
Screen::~Screen() {
	//warn << "Screen dtor called\n";   // warn called but no string passed; prob due to shutting down app

	if (initialized()) {
		// Following call takes care of unmap. See v3d_bo_free() in v3d_bufmgr.c
		v3d_bufmgr_destroy_w(p_screen);
		p_screen = nullptr;
	}
}


Screen screen;  // Needs to be a singleton


void s_screen_init() {
	screen.p_screen = (struct v3d_screen *) ralloc_size(nullptr, sizeof(struct v3d_screen));
	assert(screen.p_screen != nullptr);	

	memcpy( (void *) screen.p_screen, &screen_init, sizeof(struct v3d_screen));

	list_inithead(&(screen.p_screen->bo_cache.time_list));

	// NOTE: Not cleaned up
	struct list_head *head = (struct list_head *) ralloc_size(nullptr, sizeof(struct list_head));
	list_inithead(head);
  screen.p_screen->bo_cache.size_list = head;
}

}  // anon namespace


namespace s_screen {

int  get_fd() {
	if (!screen.initialized()) return 0;
	return screen.p_screen->fd;
}


void set_fd(int val) {
	assert(screen.initialized());	
	screen.p_screen->fd = val;
}

bool fd_is_open() { return get_fd() > 0; }

struct v3d_screen *ptr() {
	assert(screen.initialized());
	return screen.p_screen;
}

void init() {
	s_screen_init();
}

}  // namespace s_screen
