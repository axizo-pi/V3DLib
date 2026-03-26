#ifndef _V3DLIB_EMULATOR_QPUSTATE_H_
#define _V3DLIB_EMULATOR_QPUSTATE_H_
#include "EmuSupport.h"
#include "Target/instr/Instr.h"
#include "DMAAddr.h"

namespace V3DLib {

class State;

/**
 * @brief Very simple queue containing N elements of type T
 */
template <int N, typename T> struct Queue {
	const int size = N;

  T elems[N+1];
  int front;
  int back;
  Queue() { front = back = 0; }
  bool isEmpty() const { return front == back; }
  bool isFull() const { return ((back+1) % size) == front; }
  void enq(T elem) { elems[back] = elem; back = (back+1) % size; }
  T* first() { return &elems[front]; }
  void deq() { front = (front+1) % size; }
};


class SFU {
public:
  bool writeReg(Reg dest, Vec v);
  void upkeep(Vec &r4);

private:
  Vec value;       // Last result of SFU unit call
  int timer = -1;  // Number of cycles to wait for SFU result.
};


/**
 * @brief State of a single QPU.
 */
struct QPUState {
  int id = 0;                          // QPU id
  int nextUniform = -2;                // Pointer to next uniform to read
  Seq<Vec> loadBuffer = 8;             // Load buffer for loads via TMU, 8 is initial size

  int readPitch = 0;                   // Read pitch
  int writeStride = 0;                 // Write stride

  bool running = false;                // Is QPU active, or has it halted?
  bool waiting = false;                // True if DMA store/load waits
  int pc = 0;                          // Program counter
  Vec* regFileA = nullptr;             // Register file A
  int sizeRegFileA = 0;                // (and size)
  Vec* regFileB = nullptr;             // Register file B
  int sizeRegFileB = 0;                // (and size)
  Vec accum[6];                        // Accumulator registers
  bool negFlags[NUM_LANES];            // Negative flags
  bool zeroFlags[NUM_LANES];           // Zero flags

  DMAAddr dmaLoad;                     // DMA load address
  DMAAddr dmaStore;                    // DMA store address
  DMALoadReq dmaLoadSetup;             // DMA load setup register
  DMAStoreReq dmaStoreSetup;           // DMA store setup register
  Queue<2, VPMLoadReq> vpmLoadQueue;   // VPM load queue
  VPMStoreReq vpmStoreSetup;           // VPM store setup

  SFU sfu;

  QPUState();
  ~QPUState();

  void init(int maxReg);
  void upkeep(State &state);
  std::string dump(int index = -1) const;
};

} // namespace V3DLib

#endif  // _V3DLIB_EMULATOR_QPUSTATE_H_
