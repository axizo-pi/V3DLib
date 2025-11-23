#ifndef _V3D_DRIVER_BOLIST_H
#define _V3D_DRIVER_BOLIST_H

#include <vector>
#include <cstdint>

class BOList : public std::vector<struct v3d_bo *> {
public:
	~BOList();

	uint32_t       add_handle(uint32_t size, bool warn_on_error = false);
	struct v3d_bo *by_handle(uint32_t handle) const;
	bool           delete_by_handle(uint32_t handle);

private:
	void unreference(struct v3d_bo *ptr);
};


#endif // _V3D_DRIVER_BOLIST_H
