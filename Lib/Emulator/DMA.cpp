#include "DMA.h"
#include "State.h"

namespace V3DLib {
namespace DMA {

void load(QPUState *s, State *g) {
  DMALoadReq* req = &s->dmaLoadSetup;

  if (req->hor) {
    //
    // Horizontal access
    //
    uint32_t y = (req->vpmAddr >> 4) & 0x3f;
    for (int r = 0; r < req->numRows; r++) {
      uint32_t x = req->vpmAddr & 0xf;
      for (int i = 0; i < req->rowLen; i++) {
        uint32_t addr = (uint32_t) (s->dmaLoad.addr.intVal + (r * s->readPitch) + i*4);
        g->vpm[y*16 + x].intVal = g->emuHeap.phy(addr >> 2);
        x = (x + 1) % 16;
      }
      y = (y + 1) % 64;
    }
  } else {
    //
    // Vertical access
    //
    uint32_t x = req->vpmAddr & 0xf;
    for (int r = 0; r < req->numRows; r++) {
      uint32_t y = ((req->vpmAddr >> 4) + r*req->vpitch) & 0x3f;
      for (int i = 0; i < req->rowLen; i++) {
        uint32_t addr = (uint32_t) (s->dmaLoad.addr.intVal + (r * s->readPitch) + i*4);
        g->vpm[y*16 + x].intVal = g->emuHeap.phy(addr >> 2);
        y = (y + 1) % 64;
      }
      x = (x + 1) % 16;
    }
  }
}


void store(QPUState *s, State *g) {
  DMAStoreReq* req = &s->dmaStoreSetup;
  uint32_t memAddr = s->dmaStore.addr.intVal;

  if (req->hor) {
    //
    // Horizontal access
    //
    uint32_t y = (req->vpmAddr >> 4) & 0x3f;
    for (int r = 0; r < req->numRows; r++) {
      uint32_t x = req->vpmAddr & 0xf;
      for (int i = 0; i < req->rowLen; i++) {
        g->emuHeap.phy(memAddr >> 2) = g->vpm[y*16 + x].intVal;
        x = (x + 1) % 16;
        memAddr = memAddr + 4;
      }
      y = (y + 1) % 64;
      memAddr += s->writeStride;
    }
  } else {
    //
    // Vertical access
    //
    uint32_t x = req->vpmAddr & 0xf;
    for (int r = 0; r < req->numRows; r++) {
      uint32_t y = (req->vpmAddr >> 4) & 0x3f;
      for (int i = 0; i < req->rowLen; i++) {
        g->emuHeap.phy(memAddr >> 2) = g->vpm[y*16 + x].intVal;
        y = (y + 1) % 64;
        memAddr = memAddr + 4;
      }
      x = (x + 1) % 16;
      memAddr += s->writeStride;
    }
  }
}

} // namespace DMA
} // namespace V3DLib

