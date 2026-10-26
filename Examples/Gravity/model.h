#ifndef _V3DLIB_GRAVITY_MODEL_H
#define _V3DLIB_GRAVITY_MODEL_H
#include "V3DLib.h"
#include "Support/bmp.h"
#include "defaults.h"

using namespace V3DLib;

/**
 * Shared buffers and other data for the kernels
 *
 * ------
 *
 * NOTES
 * -----
 *
 * 1. Adding the dummy buffer **at the end* prevents following error:
 *
 *      ERROR: mbox_property() ioctl failed, ret: -1, error: Unknown error -1
 *      ERROR: execute_qpu(): mbox_property call failed
 *
 * What happened is that the DMA write on the _last_ buffer in the model failed.
 *
 * This because of how DMA works:
 * - Only the first value (index 0) in a vector value is used
 * - All other pointer values are consecutive from first.
 *
 * If the DMA write is defined properly, buffer `dummy` can be removed.
 */
struct Model {
  Float::Array x;
  Float::Array y;
  Float::Array z;
  Float::Array v_x;
  Float::Array v_y;
  Float::Array v_z;
  Float::Array mass;

  Float::Array acc_x;
  Float::Array acc_y;
  Float::Array acc_z;

  Float::Array dummy;           // See Note 1

  Image orbit_img{512, 512};    //> Orbit image

  void init();
  void plot();
  void save_orbits();
  void save_animate(bool force = false);
  std::string dump_pos() const;
  std::string dump_acc() const;

private:
	const int ANIMATE_SKIP = 360;  //> Number of days to skip for every animate output
	int animate_count      = 0;    //> Number of times `save_animate()` is called
	int index              = 0;    //> Prefix index for animate

	void plot_img(Image &img);
};

#endif //  _V3DLIB_GRAVITY_MODEL_H
