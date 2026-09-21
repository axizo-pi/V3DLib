#ifndef _LIB_COMMON_INSTRUCTIONCOMMENT_H
#define _LIB_COMMON_INSTRUCTIONCOMMENT_H
#include <string>

namespace V3DLib {

/**
 * @brief Mixin for instruction comments.
 *
 * Layouts of comments are as follows:
 *
 *     #
 *     # header
 *     #
 *     107: sub.pushn -, rf1, rf0         ; nop                      # comment
 *
 *     # sub_header
 *     108: sub.pushz -, r2, 0            ; nop
 *     # --- footer  ---
 *
 * All current comments can be combined for a single instruction.
 */
class InstructionComment {
public:
  InstructionComment();

  void transfer_comments(InstructionComment const &rhs);
  void clear_comments();
  bool has_comments() const;
  bool transferred() const;

  std::string const &header() const;
  std::string const &comment() const;

  std::string emit_comments(
    std::string const &line,
    std::string const &comment_prefix = "#",
    int max_size = -1
  ) const;

protected:
  void header(std::string const &msg);
  void sub_header(std::string const &msg);
  void comment(std::string msg);
  void footer(std::string msg);

private:
  std::string m_header_1;
  std::string m_header_2;
  std::string m_comment;
  std::string m_footer;
  mutable bool m_transferred = false;


  std::string emit_header(std::string const &comment_prefix = "#") const;
  std::string emit_comment(int instr_size, int max_size = -1, std::string const &comment_prefix = "#") const;
  std::string emit_footer(std::string const &comment_prefix = "#") const;
};

}  // namespace V3DLib

#endif  // _LIB_COMMON_INSTRUCTIONCOMMENT_H
