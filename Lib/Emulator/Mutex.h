#ifndef _V3DLIB_EMULATOR_MUTEX_H_
#define _V3DLIB_EMULATOR_MUTEX_H_

namespace V3DLib {
namespace Mutex {

bool acquire(int qpu_number);
bool release(int qpu_number);
bool blocks(int qpu_number);

} // namespace Mutex
} // namespace V3DLib

#endif  // _V3DLIB_EMULATOR_MUTEX_H_
