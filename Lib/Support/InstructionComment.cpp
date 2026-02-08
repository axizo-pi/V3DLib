#include "InstructionComment.h"
#include "Support/basics.h"

namespace V3DLib {

void InstructionComment::transfer_comments(InstructionComment &rhs) {
  if (!rhs.header().empty()) {
		//Log::warn << "transfer_comments header: " << rhs.header();
    header(rhs.header());
  }

  if (!rhs.comment().empty()) {
		//Log::warn << "transfer_comments comment: " << rhs.comment();
    comment(rhs.comment());
  }

	rhs.m_transferred = true;
}


bool InstructionComment::transferred() const {
	// Don't bother signalling if no comments present
	if (header().empty() && comment().empty()) return true;

	return m_transferred;
}


void InstructionComment::clear_comments() {
  m_header.clear();
  m_comment.clear();
}


bool InstructionComment::has_comments() const {
	return !m_header.empty() || !m_comment.empty();
}


/**
 * Assign header comment to current instance
 *
 * For display purposes only, when generating a dump of the opcodes.
 */
void InstructionComment::header(std::string const &msg) {
  if (msg.empty()) return;
  assertq(m_header.empty(), "Header comment already has a value when setting it", true);

  m_header = msg;
  findAndReplaceAll(m_header, "\n", "\n# ");
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

  findAndReplaceAll(msg, "\n", "\n# ");

  if (!m_comment.empty()) {
    m_comment += "; ";
  }

  m_comment += msg;
}


std::string InstructionComment::emit_header() const {
  if (m_header.empty()) return "";

  std::string ret;
  ret << "\n#\n# " << header() << "\n#\n";
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
std::string InstructionComment::emit_comment(int instr_size, int max_size) const {
  if (m_comment.empty()) return "";

  const int COMMENT_INDENT = 60;

  if (max_size == -1 ) max_size = COMMENT_INDENT;

  int spaces = 2 + max_size - instr_size;
  if (spaces < 2) spaces = 2;

  std::string ret;
  ret << tabs(spaces) << "# " << m_comment;
  return ret;
}

}  // namespace V3DLib
