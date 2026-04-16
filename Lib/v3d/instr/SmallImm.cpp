#include "SmallImm.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Target/instr/Imm.h"
#include <stdio.h>
#include <vector>

namespace V3DLib {
namespace v3d {
namespace instr {
namespace {

struct float_encoding {
  float_encoding(int in_encoding, float in_val, uint32_t in_hex = 0) :
    encoding(in_encoding),
    val(in_val),
    hex(in_hex)
  {}

  int encoding;
  float val;
  uint32_t hex;
};

std::vector<float_encoding> float_encodings = {
 {     0,   0.0f      }, // 0, same as int 0
 {    32,   1.0f      }, // 0x3f800000
 {    33,   2.0f      }, // 0x40000000
 {    34,   4.0f      },
 {    35,   8.0f      },
 {    36,  16.0f      },
 {    37,  32.0f      },
 {    38,  64.0f      }, // 0x42800000
 {    39, 128.0f      }, // 0x43000000
 {    40, 1.0f/256.0f }, // 0x3b800000
 {    41, 1.0f/128.0f }, // 0x3c000000
 {    42, 1.0f/64.0f  },
 {    43, 1.0f/32.0f  },
 {    44, 1.0f/16.0f  },
 {    45, 1.0f/8.0f   },
 {    46, 1.0f/4.0f   }, // 0x3e800000
 {    47, 1.0f/2.0f   }, // 0x3f000000
/*
48   Mul output vector rotation is taken from accumulator r5, element 0, bits [3:0]
49   Mul output vector rotated by 1 upwards (so element 0 moves to element 1)
50   Mul output vector rotated by 2 upwards (so element 0 moves to element 2)
…
62   Mul output vector rotated by 14 upwards (so element 0 moves to element 14)
63   Mul output vector rotated by 15 upwards (so element 0 moves to element 15)
*/
};


//
// Small imm encodings are different on v3d
//
std::vector<float_encoding> float_encodings_v3d = {
 {  0,   0          , 0x00000000 },  // 0, same as int 0
 { 32,   0.00390625f, 0x3b800000 },  // 2.0^-8
 { 33,   0.0078125f , 0x3c000000 },  // 2.0^-7
 { 34,   0.015625f  , 0x3c800000 },  // 2.0^-6
 { 35,   0.03125f   , 0x3d000000 },  // 2.0^-5
 { 36,   0.0625f    , 0x3d800000 },  // 2.0^-4
 { 37,   0.125f     , 0x3e000000 },  // 2.0^-3
 { 38,   0.25f      , 0x3e800000 },  // 2.0^-2
 { 39,   0.5f       , 0x3f000000 },  // 2.0^-1
 { 40,   1          , 0x3f800000 },  // 2.0^0
 { 41,   2          , 0x40000000 },  // 2.0^1
 { 42,   4          , 0x40800000 },  // 2.0^2
 { 43,   8          , 0x41000000 },  // 2.0^3
 { 44,  16          , 0x41800000 },  // 2.0^4
 { 45,  32          , 0x42000000 },  // 2.0^5
 { 46,  64          , 0x42800000 },  // 2.0^6
 { 47, 128          , 0x43000000 },  // 2.0^7
};

//
// From  vc4 reference
//
struct int_encoding {
  int_encoding(int in_encoding, int in_val) : encoding(in_encoding), val(in_val) {}

  int encoding;
  int val;
};

std::vector<int_encoding> int_encodings = {
  {  0,   0 },
  {  1,   1 },
  {  2,   2 },
  {  3,   3 },
  {  4,   4 },
  {  5,   5 },
  {  6,   6 },
  {  7,   7 },
  {  8,   8 },
  {  9,   9 },
  { 10,  10 },
  { 11,  11 },
  { 12,  12 },
  { 13,  13 },
  { 14,  14 },
  { 15,  15 },
  { 16, -16 },
  { 17, -15 },
  { 18, -14 },
  { 19, -13 },
  { 20, -12 },
  { 21, -11 },
  { 22, -10 },
  { 23,  -9 },
  { 24,  -8 },
  { 25,  -7 },
  { 26,  -6 },
  { 27,  -5 },
  { 28,  -4 },
  { 29,  -3 },
  { 30,  -2 },
  { 31,  -1 },
};

}  // anon namepace


/**
 * Do a reverse lookup from encoding to int.
 */
int SmallImm::to_int() const {
  assert(m_val != 0xff);

  bool found_it  = false;
  int ret = 0;

  for (auto &item : int_encodings) {
    if (item.encoding == m_val) {
      ret = item.val;
      found_it = true;
      break;
    }
  }

  assert(found_it);
  return ret;

}

/**
 * @return true if conversion succeeded, false otherwise
 */
bool SmallImm::int_to_opcode_value(int value, int &rep_value) {
  bool found_it  = false;

  for (auto &item : int_encodings) {
    if (item.val == value) {
      rep_value = item.encoding;
      found_it = true;
      break;
    }
  }

  if (!found_it) rep_value = INVALID_ENCODING;

  return found_it;
}


/**
 * @return true if conversion succeeded, false otherwise
 */
bool SmallImm::float_to_opcode_value(float value, int &rep_value) {
  bool found_it  = false;

  auto const &encodings = Platform::compiling_for_vc4()?float_encodings:float_encodings_v3d;

  for (auto &item : encodings) {
    if (item.val == value) {
      rep_value = item.encoding;
      found_it = true;
      break;
    }
  }

  if (!found_it) rep_value = INVALID_ENCODING;

  return found_it;
}


SmallImm::SmallImm(int val) {
  int rep_value;

  // pack calls a mesa function which does essentially the same thing
  if (int_to_opcode_value(val, rep_value)) {
    m_val = (uint8_t) rep_value;
    return;
  }

  // It might be an int encoded float value
  float val2 = *((float *) &val);

  if (float_to_opcode_value(val2, rep_value)) {
    m_val = (uint8_t) rep_value;
    return;
  }

  cerr << "SmallImm ctor, int conversion to opcode failed. val: " << val << thrw;
}


SmallImm::SmallImm(float val) {
  int rep_value;

  if (!float_to_opcode_value(val, rep_value)) {
    cerr << "SmallImm ctor, float conversion to opcode failed. val: " << val;
    breakpoint;
  }

  m_val = (uint8_t) rep_value;
}


SmallImm::SmallImm(const Imm &val) {
  // Assume that val is not encoded

  int rep_value;

  if (val.is_int()) {
    if (!int_to_opcode_value(val.intVal(), rep_value)) {
      breakpoint;
    }
    //assert(is_legal_encoded_value(m_val));
    m_val = (uint8_t) rep_value;
  } else {
    // Must be float
    if (!float_to_opcode_value(val.floatVal(), rep_value)) {
      breakpoint;
    }

    m_val = (uint8_t) rep_value;
  }

  if (!(0 <= m_val && m_val < 64)) {
    breakpoint;
  }
}


bool SmallImm::is_legal_encoded_value(int value) {
  for (auto &item : int_encodings) {
    if (item.encoding == value) {
      return true;
    }
  }

  auto const &encodings = Platform::compiling_for_vc4()?float_encodings:float_encodings_v3d;

  for (auto &item : encodings) {
    if (item.encoding == value) {
      return true;
    }
  }

  return false;
}


std::string SmallImm::print_encoded_value(int value) {
  std::string ret;

  for (auto &item : int_encodings) {
    if (item.encoding == value) {
      ret << item.val;
    }
  }

  auto const &encodings = Platform::compiling_for_vc4()?float_encodings:float_encodings_v3d;

  for (auto &item : encodings) {
    if (item.encoding == value) {
      ret << item.val;
      break;
    }
  }

  if (ret.empty()) {
    ret << "<INVALID_ENCODING>";
  }

  return ret;
}


uint8_t SmallImm::val() const {
  assertq(m_val != 0xff, "Value not set");
  return m_val;
}


bool SmallImm::operator==(SmallImm const &rhs) const {
  assertq(m_val != 0xff, "Incorrect index value");
  assertq(rhs.m_val != 0xff, "Incorrect index value rhs");
  return m_val == rhs.m_val;
}


SmallImm &SmallImm::l() {
  m_input_unpack = V3D_QPU_UNPACK_L;
  return *this;
}


SmallImm &SmallImm::ff() {
  m_input_unpack = V3D_QPU_UNPACK_REPLICATE_32F_16;
  return *this;
}


std::string SmallImm::dump() const {
  std::string ret;
  ret << "val: " << m_val << ", input_unpack: " << m_input_unpack; 

  return ret;
}

}  // instr
}  // v3d
}  // V3DLib
