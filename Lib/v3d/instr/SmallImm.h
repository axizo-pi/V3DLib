#ifndef _V3DLIB_V3D_INSTR_SMALLIMM_H
#define _V3DLIB_V3D_INSTR_SMALLIMM_H
#include <stdint.h>
#include <string>
#include "v3d_api.h"

namespace V3DLib {

class Imm;

namespace v3d {
namespace instr {


class SmallImm {
public:
  SmallImm(int val);
  SmallImm(float val);
  SmallImm(const Imm &val);

  static const int INVALID_ENCODING = -1;

  uint8_t to_raddr() const;
  v3d_qpu_input_unpack input_unpack() const { return m_input_unpack; }
  uint8_t val() const;
  bool operator==(SmallImm const &rhs) const;
  bool operator!=(SmallImm const &rhs) const { return !(*this == rhs); }

  SmallImm &l();
  SmallImm &ff();

  std::string dump() const;

  int to_int() const;

  static bool int_to_opcode_value(int value, int &rep_value);
  static bool float_to_opcode_value(float value, int &rep_value);
  static bool is_legal_encoded_value(int value);
  static std::string print_encoded_value(int value);

private:
  uint8_t m_val = 0xff;  // init to illegal value
  v3d_qpu_input_unpack m_input_unpack = V3D_QPU_UNPACK_NONE;
};

}  // instr
}  // v3d
}  // V3DLib

#endif  // _V3DLIB_V3D_INSTR_SMALLIMM_H
