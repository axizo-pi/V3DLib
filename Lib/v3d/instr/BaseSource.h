#ifndef _V3DLIB_V3D_INSTR_BASESOURCE_H
#define _V3DLIB_V3D_INSTR_BASESOURCE_H
#include <memory>  // uint8_t

namespace V3DLib {
namespace v3d {
namespace instr {

/**
 * TODO: Name is a misnomer; it is also used for dest registers.
 *       Potentially it can also be used for sig_addr.
 */
class BaseSource {
public:
  BaseSource()                             = default;
  BaseSource(BaseSource &&k)               = default;
  BaseSource &operator=(const BaseSource&) = default;

  bool operator==(const BaseSource &rhs) const;
  bool operator!=(const BaseSource &rhs) const {return !(*this == rhs); }

  std::string dump() const;
  void set_from_src(uint8_t val, bool is_small_imm, bool is_reg, bool is_rfa);
  void set_from_dst(uint8_t val, bool is_magic);

  uint8_t val()       const { return m_val; }
  bool is_set()       const { return m_is_set; }
  bool is_small_imm() const { return m_is_small_imm; }
  bool is_reg()       const { return m_is_reg; }
  bool is_rfa()       const { return m_is_rfa; }
  bool is_magic()     const { return m_is_magic; }

private:
  bool    m_is_set       = false;
  uint8_t m_val          = 0;      // Depending on bool's, rf or mux or small imm
  bool    m_is_small_imm = false;
  bool    m_is_reg       = false;
  bool    m_is_rfa       = false;  // false means rfb
  bool    m_is_dst       = false;
  bool    m_is_magic     = false;

};


}  // namespace instr
}  // namespace v3d
}  // namespace V3DLib


#endif // _V3DLIB_V3D_INSTR_BASESOURCE_H
