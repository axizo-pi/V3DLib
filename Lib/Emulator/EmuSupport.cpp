#include "EmuSupport.h"
#include "Source/Op.h"
#include <cmath>

/** \file 
 * EmuSupport 
 * ==========
 *
 * Helper functions for running the emulator.
 */

namespace V3DLib {
namespace {

// Bitwise rotate-right
inline int32_t rotRight(int32_t x, int32_t n) {
  uint32_t ux = (uint32_t) x;
  return (ux >> n) | (x << (32-n));
}


// Count leading zeros
inline int32_t clz(int32_t x) {
  int32_t count = 0;
  int32_t n = (int32_t) (sizeof(int)*8);
  for (int32_t i = 0; i < n; i++) {
    if (x & (1 << (n-1))) break;
    else count++;
    x <<= 1;
  }

  return count;
}


/**
 * Rotate a vector
 */
Vec rotate(Vec v, int n) {
  Vec w;
  for (int i = 0; i < NUM_LANES; i++)
    w[(i+n) % NUM_LANES] = v[i];
  return w;
}

}  // anon namespace


// ============================================================================
// Struct Vec
// ============================================================================

Vec::Vec(int val) {
  for (int i = 0; i < NUM_LANES; i++)
    elems[i].intVal = val;
}


Vec::Vec(Imm imm) {
  switch (imm.tag()) {
    case Imm::IMM_INT32:
      for (int i = 0; i < NUM_LANES; i++)
        (*this)[i].intVal = imm.intVal();
      break;

    case Imm::IMM_FLOAT32:
      for (int i = 0; i < NUM_LANES; i++)
        (*this)[i].floatVal = imm.floatVal();
      break;

    case Imm::IMM_MASK:
      for (int i = 0; i < NUM_LANES; i++)
        (*this)[i].intVal = (imm.mask() >> i) & 1;
      break;

    default: assert(false); break;
  }
}


Vec::Vec(std::vector<int> const &rhs) {
  assert(rhs.size() == NUM_LANES);

  for (int i = 0; i < NUM_LANES; i++) {
    (*this)[i].intVal = rhs[i];
  }
}


Vec &Vec::operator=(int rhs) {
  for (int i = 0; i < NUM_LANES; i++) {
    (*this)[i].intVal = rhs;
  }

  return *this;
}


Vec &Vec::operator=(float rhs) {
  for (int i = 0; i < NUM_LANES; i++) {
    (*this)[i].floatVal = rhs;
  }

  return *this;
}


bool Vec::operator==(Vec const &rhs) const {
  for (int i = 0; i < NUM_LANES; i++) {
    if ((*this)[i].intVal != rhs[i].intVal) return false;
  }

  return true;
}


bool Vec::operator==(int rhs) const {
  breakpoint

  for (int i = 0; i < NUM_LANES; i++) {
    if ((*this)[i].intVal != rhs) return false;
  }

  return true;
}


std::string Vec::dump() const {
  std::string ret;
  ret << "<";

  for (int i = 0; i < NUM_LANES; i++) {
    ret << elems[i].intVal << ", ";
  }

  ret << ">";
  return ret;
}


/**
 * Negate a condition vector
 */
Vec Vec::negate() const {
  Vec v;

  for (int i = 0; i < NUM_LANES; i++)
    v[i].intVal = !elems[i].intVal;
  return v;
}


Vec Vec::recip() const {
  Vec ret;

  for (int i = 0; i < NUM_LANES; i++) {
    float a = elems[i].floatVal;
    ret[i].floatVal = (a != 0)?1/a:0;      // TODO: not sure about value safeguard
  }

  return ret;
}


Vec Vec::recip_sqrt() const {
  Vec ret;

  for (int i = 0; i < NUM_LANES; i++) {
    float a = (float) ::sqrt(elems[i].floatVal);
    ret[i].floatVal = (a != 0)?1/a:0;      // TODO: not sure about value safeguard
  }

  return ret;
}


Vec Vec::exp() const {
  Vec ret;

  for (int i = 0; i < NUM_LANES; i++) {
    ret[i].floatVal = (float) ::exp2(elems[i].floatVal);
  }

  return ret;
}


Vec Vec::log() const {
  Vec ret;

  for (int i = 0; i < NUM_LANES; i++) {
    float a = (float) ::log2(elems[i].floatVal);  // TODO what would log2(0) return?
    ret[i].floatVal = a;
  }

  return ret;
}


bool Vec::apply(Op const &op, Vec a, Vec b) {
  bool handled = true;

  switch (op.op) {
    case RECIP    : *this = a.recip();      break;
    case RECIPSQRT: *this = a.recip_sqrt(); break;
    case EXP      : *this = a.exp();        break;
    case LOG      : *this = a.log();        break;
    default: handled = false;
   }

  if (handled) return true;
  return apply(ALUOp(op), a, b);
}


bool Vec::apply(ALUOp const &op, Vec a, Vec b) {
  bool handled = true;
  if (op.value() == Enum::NOP) return true;

  // Floating-point operations
  for (int i = 0; i < NUM_LANES; i++) {
    float  x = a[i].floatVal;
    float  y = b[i].floatVal;
    float &d = elems[i].floatVal;

    switch (op.value()) {
    case Enum::A_FADD:    d = x+y;                       break;
    case Enum::A_FSUB:    d = x-y;                       break;
    case Enum::A_FMIN:    d = x<y?x:y;                   break;
    case Enum::A_FMAX:    d = x>y?x:y;                   break;
    case Enum::A_FMINABS: d = fabs(x) < fabs(y) ? x : y; break; // min of absolute values
    case Enum::A_FMAXABS: d = fabs(x) > fabs(y) ? x : y; break; // max of absolute values
    case Enum::A_FtoI:    elems[i].intVal = (int) x;     break;
    case Enum::A_ItoF:    d = (float) a[i].intVal;       break;
    case Enum::M_FMUL:    d = x*y;                       break;

    default:
      handled = false;
      break;
    }
  }

  if (handled) return handled;
  handled = true;

  // Integer operations
  for (int i = 0; i < NUM_LANES; i++) {
    int  x = a[i].intVal;
    int  y = b[i].intVal;
    int &d = elems[i].intVal;

    switch (op.value()) {
    case Enum::A_ADD:   d = x+y;            break;
    case Enum::A_SUB:   d = x-y;            break;
    case Enum::A_ROR:   d = rotRight(x, y); break;
    case Enum::A_SHL: {
      d = x << y;
      //warn << "Vec::apply() A_SHL: d = x << y: " << d << " = " << x << " << " <<  y;
    }
    break;
    case Enum::A_SHR:   d = (int32_t) (((uint32_t) x) >> y); break;
    case Enum::A_ASR:   d = x >> y; break;
    case Enum::A_MIN:   d = x<y?x:y;        break;
    case Enum::A_MAX:   d = x>y?x:y;        break;
    case Enum::A_BAND:  d = x&y;            break;
    case Enum::A_BOR:   d = x|y;            break;
    case Enum::A_BXOR:  d = x^y;            break;
    case Enum::A_BNOT:  d = ~x; break;
    case Enum::M_MUL24: {                           // Integer multiply (24-bit)
      int x2 = (x & 0xffffff);  // Clip to 24 bits
      int y2 = (y & 0xffffff);

      if (x != x2) {
        cerr << "EmuSupport MUL24: var x, clipped value "  << x2 << " is different from input value " << x;
      }

      if (y != y2) {
        cerr << "EmuSupport MUL24: var y, clipped value "  << y2 << " is different from input value " << y;
      }

      d = x2*y2;
    }
    break;

    case Enum::A_CLZ:    d = clz(x);         break; // Count leading zeros

    case Enum::A_V8ADDS:
    case Enum::A_V8SUBS:
    case Enum::M_V8MUL:
    case Enum::M_V8MIN:
    case Enum::M_V8MAX:
    case Enum::M_V8ADDS:
    case Enum::M_V8SUBS: {
      cerr << "EmuSupport: unsupported operator " << op.value() << thrw;
    }
    break;

    default:
      handled = false;
      break;
    }
  }

  if (handled) return handled;
  handled = true;

  // Other operations
  switch (op.value()) {
    case Enum::M_ROTATE: { // Vector rotation
      assert(b.is_uniform());
      int n = b[0].intVal;

      assign(rotate(a,n));
    }
    break;

    case Enum::A_MOV:  // v3d
      *this = a;
    break;

    default:
      handled = false;
    break;
  }

  assertq(handled, "Vec::apply(): Unhandled op value");
  return handled;
}


/**
 * Check if all vector elements of current have the same value
 *
 * @return true if all elements same value, false otherwise
 */
bool Vec::is_uniform() const {
  int val = elems[0].intVal;

  for (int i = 1; i < NUM_LANES; i++) {
    if (elems[i].intVal != val) return false;
  }

  return true;
}


void Vec::assign(Vec const &rhs) {
  for (int i = 0; i < NUM_LANES; i++) {
    elems[i] = rhs[i];
  }
}

void vpm_read(VPMLoadReq &req, Word const *vpm, Vec &v) {
  assert(req.numVecs > 0);

  if (req.hor) {
    // Horizontal load
    for (int i = 0; i < NUM_LANES; i++) {
      int index = (16*req.addr + i);
      assert(index < VPM_SIZE);
      v[i] =vpm[index];
    }
  } else {
    // Vertical load
    for (int i = 0; i < NUM_LANES; i++) {
      uint32_t x = req.addr & 0xf;
      uint32_t y = req.addr >> 4;
      int index = (y*16*16 + x + i*16);
      assert(index < VPM_SIZE);
      v[i] = vpm[index];
    }
  }

  req.numVecs--;
  req.addr += req.stride;
}


void vpm_write(VPMStoreReq &req, Word *vpm, Vec const &v) {
  assert(vpm != nullptr);

  if (req.hor) {
    // Horizontal store
    for (int i = 0; i < NUM_LANES; i++) {
      int index = (16*req.addr + i);
      assert(index < VPM_SIZE);
      vpm[index] = v[i];
    }
  } else {
    // Vertical store
    uint32_t x = req.addr & 0xf;
    uint32_t y = req.addr >> 4;
    for (int i = 0; i < NUM_LANES; i++) {
      int index = (y*16*16 + x + i*16);
      assert(index < VPM_SIZE);
      vpm[index] = v[i];
    }
  }

  req.addr += req.stride;
}

}  // namespace V3DLib
