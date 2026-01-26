#ifndef _V3DLIB_GRAVITY_MODEL_H
#define _V3DLIB_GRAVITY_MODEL_H
#include "V3DLib.h"
#include "Support/bmp.h"
#include "defaults.h"

using namespace V3DLib;

struct Model {
	Float::Array x{NUM};
	Float::Array y{NUM};
	Float::Array z{NUM};
	Float::Array v_x{NUM};
	Float::Array v_y{NUM};
	Float::Array v_z{NUM};
	Float::Array mass{NUM};

	Float::Array acc_x{NUM};
	Float::Array acc_y{NUM};
	Float::Array acc_z{NUM};

	Image img{512, 512};

	void init();
	void plot();
	void save_img();
	std::string dump_pos() const;
	std::string dump_acc() const;
};

#endif //  _V3DLIB_GRAVITY_MODEL_H

