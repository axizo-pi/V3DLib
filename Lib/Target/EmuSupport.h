#ifndef _V3DLIB_TARGET_EMUSUPPORT_H_
#define _V3DLIB_TARGET_EMUSUPPORT_H_
#include <stdint.h>
#include <vector>
#include "Common/Seq.h"
#include "instr/Imm.h"
#include "SmallLiteral.h"  // Word


/**
 * Definitions which are used in both the emulator and the interpreter.
 */
namespace V3DLib {

class Op;
class ALUOp;

const int NUM_LANES =   16;
const int MAX_QPUS  =   12;
const int VPM_SIZE  = 1024;


/**
 * @brief Vector values
 */
struct Vec {
  Vec() = default;
  Vec(int val);
  Vec(Imm imm);
  Vec(std::vector<int> const &rhs);

  Vec &operator=(Vec const &rhs) { assign(rhs); return *this; }
  Vec &operator=(int rhs);
  Vec &operator=(float rhs);

  bool operator==(Vec const &rhs) const;
  bool operator!=(Vec const &rhs) const { return !(*this == rhs); }
  bool operator==(int rhs) const;
  bool operator!=(int rhs) const { return !(*this == rhs); }

  Word &get(int index) {
    assert(0 <= index && index < NUM_LANES);
    return elems[index];
  }

  Word &operator[](int index) {
    return get(index);
  }

  Word operator[](int index) const {
    assert(0 <= index && index < NUM_LANES);
    return elems[index];
  }

  std::string dump() const;
  Vec negate() const;
  bool apply(Op const &op, Vec a, Vec b);
  bool apply(ALUOp const &op, Vec a, Vec b);
  bool is_uniform() const;

  Vec recip() const;
  Vec recip_sqrt() const;
  Vec exp() const;
  Vec log() const;

private:
  Word elems[NUM_LANES];

  void assign(Vec const &rhs);
};


class EmuState {
public:
  int num_qpus;
  Word vpm[VPM_SIZE];      // Shared VPM memory

  EmuState(int in_num_qpus, IntList const &in_uniforms, bool add_dummy = false);
  Vec get_uniform(int id, int &next_uniform);
  bool sema_inc(int sema_id);
  bool sema_dec(int sema_id);

  std::string dump_vpm() const;

  static Vec const index_vec;

private:
  IntList uniforms;        // Kernel parameters
  int sema[16];            // Semaphores

  // Protection against locks due to semaphore waiting
  int const MAX_SEMAPHORE_WAIT = 1024;
  int semaphore_wait_count = 0;
};

namespace Mutex {

bool acquire(int qpu_number);
bool release(int qpu_number);
bool blocks(int qpu_number);

} // namespace Mutex


/**
 * @brief In-flight DMA request
 */
struct DMAAddr {
  Word addr;

  bool active() const   { return m_active; }
  void active(bool val) { m_active = val; }
  void start();
  bool waiting() const  { return m_wait_count > 0; }

  void upkeep();
  std::string dump() const;

private:
  const int WaitCount = 4;  // Educated quess

  bool m_active     = false;
  int  m_wait_count = 0;
};


/**
 * @brief VPM load request
 */
struct VPMLoadReq {
  int numVecs;  // Number of vectors to load
  bool hor;     // Horizintal or vertical access?
  int addr;     // Address in VPM to load from
  int stride;   // Added to address after every vector read
};


/**
 * @brief VPM store request
 */
struct VPMStoreReq {
  bool hor;     // Horizontal or vertical access?
  int addr;     // Address in VPM to load from
  int stride;   // Added to address after every vector written
};


/**
 * @brief DMA load request
 */
struct DMALoadReq {
  bool hor;     // Horizintal or vertical access?
  int numRows;  // Number of rows in memory
  int rowLen;   // Length of each row in memory
  int vpmAddr;  // VPM address to write to
  int vpitch;   // Added to vpmAddr after each vector loaded
};


/**
 * @brief DMA store request
 */
struct DMAStoreReq {
  bool hor;     // Horizintal or vertical access?
  int numRows;  // Number of rows in memory
  int rowLen;   // Length of each row in memory
  int vpmAddr;  // VPM address to load from
};

}  // namespace V3DLib

#endif  // _V3DLIB_TARGET_EMUSUPPORT_H_
