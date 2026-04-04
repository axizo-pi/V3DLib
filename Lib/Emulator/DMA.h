#ifndef _V3DLIB_EMULATOR_DMA_H_
#define _V3DLIB_EMULATOR_DMA_H_

namespace V3DLib {

class QPUState;
class State;

namespace DMA {

void load(QPUState *s, State *g);
void store(QPUState *s, State *g);

} // namespace DMA
} // namespace V3DLib


#endif  //  _V3DLIB_EMULATOR_DMA_H_
