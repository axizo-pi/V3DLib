#include "InstructionComment.h"
#include "Support/basics.h"

namespace V3DLib {

InstructionComment::InstructionComment() :
	m_header_1(""),
	m_header_2(""),
	m_comment("")
{}


void InstructionComment::transfer_comments(InstructionComment const &rhs) {
  header(rhs.m_header_1);
  sub_header(rhs.m_header_2);
  comment(rhs.comment());
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
  assert(!m_transferred);
}


bool InstructionComment::has_comments() const {
  return !m_header_1.empty() || !m_header_2.empty() || !m_comment.empty();
}


/**
 * Note that only top-level header is returned.
 */
std::string const &InstructionComment::header()  const { return m_header_1; }

std::string const &InstructionComment::comment() const { return m_comment; }

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

} // anon namespace


void InstructionComment::header(std::string const &msg) {
	assign_header(m_header_1, msg);
}


void InstructionComment::sub_header(std::string const &msg) {
	assign_header(m_header_2, msg);
}


/**
 * Assign comment to current instance
 *
 * If a comment is already present, the new comment will be appended.
 *
 * For display purposes only, when generating a dump of the opcodes.
 */
void InstructionComment::comment(std::string msg) {
  if (msg.empty()) return;

  auto prev = m_comment;
  m_comment = msg;

   if (!prev.empty()) {
    //warn << "comment() comment already present: '" << prev << "'; adding: '" << msg << "'";
    m_comment <<  "; " << prev;
  }
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

}  // namespace V3DLib
