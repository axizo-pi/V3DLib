#ifndef _V3DLIB_V3D_KERNELDRIVER_H
#define _V3DLIB_V3D_KERNELDRIVER_H
//#include "Compile.h"
//#include "../KernelDriver.h"
#include "UniformConstants.h"
#include "Common/SharedArray.h"
#include "BufferObject.h"
#include "Driver.h"

namespace V3DLib {
namespace v3d {

class Compile; // Forward declaration

/**
 * @brief Buffer Object for v3d
 *
 * The generated code is loaded into a separate BO.
 * This is to be able to load and run multiple kernels for v3d in the same context.
 *
 * Loading multiple kernels into the same Buffer Object (BO) didn't work for unfathomable reasons,
 * and resulted in run timeouts and eventually locked up the pi4.
 * vc4 does not have this issue.
 */
class KernelDriver {
  //using Instruction  = V3DLib::v3d::instr::Instr;
  //using Instructions = V3DLib::v3d::Instructions;

public:
  KernelDriver() {}
  KernelDriver(KernelDriver &&a) = default;

  void invoke(
		Compile &code,
		int numQPUs,
		IntList &params,
  	UniformConstants const &uc,
		bool wait_complete = true
	);

  void wait_complete();

private:
  Data   uniforms;
  Data   devnull;
  Data   done;
  Driver drv;
};

}  // namespace v3d
}  // namespace V3DLib

#endif  // _V3DLIB_V3d_KERNELDRIVER_H
