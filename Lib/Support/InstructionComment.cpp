#include "InstructionComment.h"
#include "Support/basics.h"

namespace V3DLib {

InstructionComment::InstructionComment() :
	m_header(""),
	m_comment(""),
	m_main_header(true)
{}

/*
// Experimental. Default ctor is prob good enough
//TODO: is it needed?
InstructionComment::InstructionComment(InstructionComment const &rhs) :
	m_header(rhs.m_header),
	m_comment(rhs.m_comment),
	m_main_header(rhs.m_main_header)
{
	//warn << "InstructionComment ctor(rhs)";
}
*/


void InstructionComment::transfer_comments(InstructionComment const &rhs) {
  if (!rhs.header().empty()) {
  	if (header().empty()) {
			m_main_header = rhs.m_main_header;  // Never a problem
		} else if(!m_main_header) {  // Retain main header of `this` if set
			auto prev = m_main_header;

			m_main_header = rhs.m_main_header;

			if (m_main_header != prev) {
				warn << "transfer_comments main_header changed to: " << m_main_header;
			}

		}

    header(rhs.header());
  }

  if (!rhs.comment().empty()) {
    comment(rhs.comment());
  }

  rhs.m_transferred = true;
}


bool InstructionComment::transferred() const {
  // Don't bother if no comments present
  if (header().empty() && comment().empty()) return true;

  return m_transferred;
}


void InstructionComment::clear_comments() {
  m_header.clear();
  m_comment.clear();
	m_main_header = true;
  assert(!m_transferred);
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

  if (!m_header.empty()) {
    // If input is same as current, ignore
    if (msg == m_header) return;

    warn << "header() Header comment already has a value when setting it\n"
         << "current: " << m_header << "\n"
         << "new: "     << msg      << "\n"
    ;
  }

  if (!m_header.empty()) {
    m_header << "\n";
  }

  m_header <<  msg;
}


void InstructionComment::sub_header(std::string const &msg) {
	if (header().empty()) {
		m_main_header = false;
	} else {
		assert(!m_main_header);  // Warn me if/when this happens
	}

	header(msg);
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
  if (m_header.empty()) return "";

	auto c = comment_prefix;
	std::string pre = "\n";
	pre << c << " ";

  std::string buf = header();
  findAndReplaceAll(buf, "\n", pre);

  std::string ret;

	if (m_main_header) {
	  ret << "\n" << c << pre << buf << "\n" << c << "\n";
	} else {
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
