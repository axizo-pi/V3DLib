#ifndef _V3DLIB_VC4_FUNCTIONS_H_
#define _V3DLIB_VC4_FUNCTIONS_H_

namespace V3DLib {
namespace vc4 {

void mutex_acquire();
void mutex_release();


void kernelFinish();

}  // namespace vc4
}  // namespace V3DLib

#endif //  _V3DLIB_VC4_FUNCTIONS_H_
