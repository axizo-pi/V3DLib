#ifndef _LIB_COMMON_INSTRUCTIONCOMMENT_H
#define _LIB_COMMON_INSTRUCTIONCOMMENT_H
#include <string>

namespace V3DLib {

/**
 * Mixin for instruction comments
 */
class InstructionComment {
public:
  InstructionComment();
  //InstructionComment(InstructionComment const &rhs);

  void transfer_comments(InstructionComment const &rhs);
  void clear_comments();
  bool has_comments() const;
  std::string const &header() const { return m_header; }
  std::string const &comment() const { return m_comment; }

  std::string emit_header(std::string const &comment_prefix = "#") const;
  std::string emit_comment(int instr_size, int max_size = -1, std::string const &comment_prefix = "#") const;
  bool transferred() const;

protected:
  void header(std::string const &msg);
  void sub_header(std::string const &msg);
  void comment(std::string msg);

private:
  std::string m_header;
  std::string m_comment;
	bool m_main_header;
  mutable bool m_transferred = false;
};

}  // namespace V3DLib

#endif  // _LIB_COMMON_INSTRUCTIONCOMMENT_H
