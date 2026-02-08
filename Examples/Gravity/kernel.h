#ifndef _V3DLIB_GRAVITY_KERNEL_H
#define _V3DLIB_GRAVITY_KERNEL_H
#include "V3DLib.h"

using namespace V3DLib;

///////////////////////////////////////////////////////////////
// Kernel version 
///////////////////////////////////////////////////////////////

void kernel_gravity(
	Float::Ptr in_x, Float::Ptr in_y, Float::Ptr in_z,
	Float::Ptr in_v_x, Float::Ptr in_v_y, Float::Ptr in_v_z,
 	Float::Ptr in_mass,
 	Float::Ptr in_acc_x, Float::Ptr in_acc_y, Float::Ptr in_acc_z,
	Int num_entities,
  Int::Ptr signal
);


#endif //  _V3DLIB_GRAVITY_KERNEL_H
