#include "InstructionComment.h"
#include "Support/basics.h"

namespace V3DLib {
namespace {

void assign_header(std::string &header, std::string const &msg) {
  if (msg.empty()) return;

  if (!header.empty()) {
    // If input is same as current, ignore
    if (msg == header) return;

    warn << "assign_header() Header comment already has a value when setting it\n"
         << "current: " << header << "\n"
         << "new: "     << msg      << "\n"
    ;
  }

  if (!header.empty()) {
    header << "\n";
  }

  header <<  msg;
}


void append_comment(std::string &dst, std::string const &msg) {
  //warn << "append_comment";

  if (msg.empty()) return;

  auto prev = dst;
  dst = msg;

   if (!prev.empty()) {
    dst <<  "; " << prev;
  }
}

} // anon namespace


InstructionComment::InstructionComment() :
  m_header_1(""),
  m_header_2(""),
  m_comment(""),
  m_footer("")
{}


void InstructionComment::transfer_comments(InstructionComment const &rhs) {
  header(rhs.m_header_1);
  sub_header(rhs.m_header_2);
  comment(rhs.m_comment);
  footer(rhs.m_footer);
  rhs.m_transferred = true;
}


bool InstructionComment::transferred() const {
  // Don't bother if no comments present
  if (header().empty() && comment().empty()) return true;

  return m_transferred;
}


void InstructionComment::clear_comments() {
  m_header_1.clear();
  m_header_2.clear();
  m_comment.clear();
  m_footer.clear();
  assert(!m_transferred);
}


bool InstructionComment::has_comments() const {
  return !(
       m_header_1.empty()
    && m_header_2.empty() 
    && m_comment.empty()
    && m_footer.empty()
  );
}


/**
 * Note that only top-level header is returned.
 */
std::string const &InstructionComment::header()  const { return m_header_1; }

std::string const &InstructionComment::comment() const { return m_comment; }


void InstructionComment::header(std::string const &msg) {
  assign_header(m_header_1, msg);
}


void InstructionComment::sub_header(std::string const &msg) {
  assign_header(m_header_2, msg);
}


/**
 * @brief Assign comment to current instance
 *
 * If a comment is already present, the new comment will be appended.
 *
 * For display purposes only, when generating a dump of the opcodes.
 */
void InstructionComment::comment(std::string msg) {
  append_comment(m_comment, msg);
}


void InstructionComment::footer(std::string msg) {
  append_comment(m_footer, msg);
}


std::string InstructionComment::emit_header(std::string const &comment_prefix) const {
  if (m_header_1.empty() && m_header_2.empty()) return "";

  auto c = comment_prefix;
  std::string pre = "\n";
  pre << c << " ";

  std::string ret;

  if (!m_header_1.empty()) {
    std::string buf = m_header_1;
    findAndReplaceAll(buf, "\n", pre);

    ret << "\n" << c << pre << buf << "\n" << c << "\n";
  }

  if (!m_header_2.empty()) {
    std::string buf = m_header_2;
    findAndReplaceAll(buf, "\n", pre);

    ret << pre << buf << "\n";
  }

  return ret;
}


std::string InstructionComment::emit_footer(std::string const &comment_prefix) const {
  if (m_footer.empty()) return "";

  auto c = comment_prefix;

  std::string ret;

  if (!m_footer.empty()) {
    ret << "\n" << c << " --- " << m_footer << "--- \n";
  }

  return ret;
}


/**
 * Return comment as string with leading spaces
 *
 * **NOTE**: this does not take into account multi-line comments (don't occur at time of writing)
 *
 * @param instr_size  size of the associated instruction in bytes
 * @param max_size    If specified, maximum instruction size of the encompassing instruction list
 */
std::string InstructionComment::emit_comment(int instr_size, int max_size, std::string const &comment_prefix) const {
  if (m_comment.empty()) return "";

  const int COMMENT_INDENT = 60;

  if (max_size == -1 ) max_size = COMMENT_INDENT;

  int spaces = 2 + max_size - instr_size;
  if (spaces < 2) spaces = 2;

  std::string ret;
  ret << tabs(spaces) << comment_prefix << " " << m_comment;
  return ret;
}


/**
 *
 * @param line           String to add comments to
 * @param comment_prefix Delimiter for comments, default `#`
 */
std::string InstructionComment::emit_comments(
  std::string const &line,
  std::string const &comment_prefix,
  int max_size
) const {
  std::string ret;

  ret << emit_header(comment_prefix)
      << line
      << emit_comment((int) line.size(), max_size, comment_prefix)
      << emit_footer(comment_prefix)
      << "\n"
  ;

  return ret;
}

}  // namespace V3DLib
