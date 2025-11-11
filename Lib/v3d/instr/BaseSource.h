#ifndef _V3DLIB_V3D_INSTR_BASESOURCE_H
#define _V3DLIB_V3D_INSTR_BASESOURCE_H
#include <memory>  // uint8_t

namespace V3DLib {
namespace v3d {
namespace instr {

class BaseSource {
public:
  BaseSource()                             = default;
  BaseSource(BaseSource &&k)               = default;
  BaseSource &operator=(const BaseSource&) = default;

  void set(uint8_t val, bool is_small_imm, bool is_reg = false);
  std::string dump() const;

  uint8_t val()       const { return m_val; }
  bool is_set()       const { return m_is_set; }
  bool is_small_imm() const { return m_is_small_imm; }
  bool is_reg()       const { return m_is_reg; }

private:
  bool m_is_set          = false;
  uint8_t m_val          = 0;     // Depending on bool's, rf or mux or small imm
  bool    m_is_small_imm = false;
  bool    m_is_reg       = false;
};


}  // namespace instr
}  // namespace v3d
}  // namespace V3DLib


#endif // _V3DLIB_V3D_INSTR_BASESOURCE_H
