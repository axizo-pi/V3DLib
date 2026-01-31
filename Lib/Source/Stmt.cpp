#include "Stmt.h"
#include "Support/basics.h"
#include "Support/Helpers.h"
#include "vc4/DMA/DMA.h"
#include "LibSettings.h"

using namespace Log;

namespace V3DLib {

using ::operator<<;  // C++ weirdness

namespace {

std::string make_comment(std::string &str) {
	std::vector<std::string> lines = split(str, "\n");

  std::string ret;
	ret << "\n#\n";
  for (int i = 0; i < (int) lines.size(); i++) {
		ret << "# " << lines[i] << "\n";
	}
	ret << "#\n";

	return ret;
}

}  // anon namespace


// ============================================================================
// Class Stmt
// ============================================================================

Stmt::~Stmt() {
/*	
	// Useful only for debug
	if (!InstructionComment::transferred()) {
		warn << "Stmt dtor comments not transferred\n"
				 << "     stmt: " << dump() << "\n"
				 << "   header: " << InstructionComment::header()  << "\n"
				 << "  comment: " << InstructionComment::comment() << "\n";
	}
*/	
}


void Stmt::append(Array const &rhs) {
  if (tag != SEQ) {
    Ptr s0;
    s0.reset(new Stmt(*this));
    auto tmp = Stmt::create(SEQ);
    tmp->m_stmts_a << s0;
    *this = *tmp;
  }

  m_stmts_a << rhs;
}


void Stmt::cond(CExpr::Ptr cond) {
  m_cond = cond;
}


BExpr::Ptr Stmt::where_cond() const {
  assert(tag == WHERE);
  assert(m_where_cond.get() != nullptr);
  return m_where_cond;
}


void Stmt::where_cond(BExpr::Ptr cond) {
  assert(tag == WHERE);
  assert(m_where_cond.get() == nullptr);  // Don't reassign
  m_where_cond = cond;
}


Expr::Ptr Stmt::assign_lhs() const {
  assert(tag == ASSIGN);
  assert(m_exp_a.get() != nullptr);
  return m_exp_a;
}


Expr::Ptr Stmt::assign_rhs() const {
  assert(tag == ASSIGN);
  assert(m_exp_b.get() != nullptr);
  return m_exp_b;
}


Expr::Ptr Stmt::address() {
  assert(tag == LOAD_RECEIVE);
  assert(m_exp_a.get() != nullptr);
  assert(m_exp_b.get() == nullptr);
  return m_exp_a;
}


bool Stmt::check_blocks() const {
  // then and else blocks may not both be empty
  if (m_stmts_a.empty() && m_stmts_b.empty()) return false;

  if (!m_stmts_a.empty()) {
    for (int i = 0; i < (int) m_stmts_a.size(); i++) {
      if (!m_stmts_a[i]) return false;
    }
  }

  if (!m_stmts_b.empty()) {
    for (int i = 0; i < (int) m_stmts_b.size(); i++) {
      if (!m_stmts_b[i]) return false;
    }
  }

  return true;
}


Stmt::Array const &Stmt::then_block() const {
	// Following is bullshit. then_block can also appear for WHILE
  //assertq(tag == IF || tag == WHERE, "Then-statement only valid for IF, and WHERE", true);
  assert(check_blocks());

  return m_stmts_a;
}


Stmt::Array const &Stmt::body() const {
  assertq(tag == SEQ || tag == WHILE || tag == FOR, "Body-statement only valid for SEQ, WHILE and FOR", true);
  assert(check_blocks());

  // must have then and no else
  assert(!m_stmts_a.empty() && m_stmts_b.empty());

  return m_stmts_a;
}


Stmt::Array const &Stmt::else_block() const {
  assertq(tag == IF || tag == WHERE, "Else-statement only valid for IF and WHERE", true);
  // where and else stmt may not both be empty
  assert(!m_stmts_a.empty() || !m_stmts_b.empty());
  return m_stmts_b; // May be null
}


/**
 * @return true if block successfully added, false otherwise
 */
bool Stmt::then_block(Array const &in_block) {
  assert((tag == Stmt::IF || tag == Stmt::WHERE ) && m_stmts_a.empty());  // TODO check if converse ever happens

  if ((tag == Stmt::IF || tag == Stmt::WHERE ) && m_stmts_a.empty()) {
    m_stmts_a << in_block;
    return true;
  }

  return false;
}


/**
 * @return true if block successfully added, false otherwise
 */
bool Stmt::add_block(Array const &block) {
  bool then_is_empty = (m_stmts_a.empty());
  bool else_is_empty = (m_stmts_b.empty());

  switch (tag) {
    case Stmt::IF:
    case Stmt::WHERE:
      if (then_is_empty) {
        then_block(block);
        return true;
      } else if (else_is_empty) {
        m_stmts_b = block;
        return true;
      } else {
        assert(false);
      }
      break;

    case Stmt::WHILE:
      if (then_is_empty) {
        m_stmts_a = block;
        return true;
      }
      break;

    case FOR:
      if (then_is_empty) {
        // convert For to While
        //m_cond retained as is
        // m_stmts_b is inc
        tag = WHILE;

        m_stmts_a << block << m_stmts_b;
        m_stmts_b.clear();
        return true;
      }
      break;

    default: assert(false); break;  // Should really never happen
  }

  return false;
}


void Stmt::inc(Array const &arr) {
  assertq(tag == FOR, "Inc-statement only valid for FOR", true);
  assert(m_stmts_b.empty());  // Only assign once
  m_stmts_b = arr;
}


/**
 * Dump output for current statement.
 *
 * @return Text representation of current statement.
 */
std::string Stmt::disp_intern(bool with_linebreaks, int seq_depth, bool show_comments) const {
  std::string ret;

  switch (tag) {
    case SKIP:
      ret << "SKIP";
    break;
    case ASSIGN:
      ret << "ASSIGN " << assign_lhs()->dump() << " = " << assign_rhs()->dump();
    break;
    case SEQ:
      assert(!m_stmts_a.empty());
      assert(m_stmts_b.empty());

      if (with_linebreaks) {
        std::string tmp;

        for (int i = 0; i < (int) m_stmts_a.size(); i++) {
          tmp << "  " << m_stmts_a[i]->disp_intern(with_linebreaks, seq_depth + 1, show_comments) << "\n";
        }

        // Remove all superfluous whitespace
        if (seq_depth == 0) {
          std::string tmp2;
          bool changed = true;

          while (changed)  {
            tmp2 = tmp;
            findAndReplaceAll(tmp2, "    ", "  ");
            findAndReplaceAll(tmp2, "\n\n", "\n");

            changed = (tmp2 != tmp);
            tmp = tmp2;
          }

          // TODO make indent based on sequence depth
          ret << "SEQ*: {\n" << tmp << "} END SEQ*\n";
        } else {
          ret << tmp;
        }
  
      } else {
        ret << "SEQ {";

        for (int i = 0; i < (int) m_stmts_a.size(); i++) {
          ret << m_stmts_a[i]->disp_intern(with_linebreaks, seq_depth + 1, show_comments) << "; ";
        }

        ret << "}";
      }
    break;
    case WHERE:
      assert(m_where_cond.get() != nullptr);
      ret << "WHERE (" << m_where_cond->dump() << ")\n"
			    << "  THEN " << then_block().dump(show_comments);
      if (!else_block().empty()) {
        ret << "\n  ELSE " << else_block().dump(show_comments);
      }
    break;
    case IF:
      assert(m_cond.get() != nullptr);
      ret << "IF (" << m_cond->dump() << ") THEN " << then_block().dump(show_comments);
      if (!else_block().empty()) {
        ret << " ELSE " << else_block().dump(show_comments);
      }
    break;
    case WHILE:
      assert(m_cond.get() != nullptr);
      if (!then_block().empty()) {
      	ret << "WHILE (" << m_cond->dump() << ")\n"
					  << "  " << then_block().dump(show_comments);
			}
      // There is no ELSE for while
    break;

    case GATHER_PREFETCH:  ret << "GATHER_PREFETCH"; break;
    case FOR:              ret << "FOR";             break;
    case LOAD_RECEIVE:     ret << "LOAD_RECEIVE";    break;
    case BARRIER:          ret << "BARRIER";         break;

    default: {
        std::string tmp = DMA::disp(tag);  // Check for unhandled DMA stuff

        if (!tmp.empty()) {
					cdebug  << "Stmt::disp_intern() default DMA: " << tmp;
        } else {
          std::string msg;
          msg << "Stmt::disp_intern() "
              << "Unknown tag '" << tag << "'";

          if (tag < 0 || tag >= NUM_TAGS) {
            msg << "; tag out of range";
          }

          assertq(false, msg, true);
				}
      }
      break;
  }

	if (show_comments) {
		std::string out;
		auto h = InstructionComment::header(); 
		if (!h.empty()) {
			out << make_comment(h);
		}

		out << ret; 

		auto c = InstructionComment::comment(); 
		if (!c.empty()) {
			out << "           ; " << c;
		}

		assert(!out.empty());
  	return out;
	}

  return ret;
}


Stmt::Ptr Stmt::create(Tag in_tag) {
  Ptr ret(new Stmt(in_tag));
  return ret;
}


Stmt::Ptr Stmt::create(Tag in_tag, Expr::Ptr e0, Expr::Ptr e1) {
  // Intention: assert(!DMA::Stmt::is_dma_tag(in_tag);  - and change default below
  Ptr ret(new Stmt(in_tag));

  switch (in_tag) {
    case ASSIGN:
      if (e0 == nullptr) {
        cerr << "Stmt::create(): e0 is null, variable might not be initialized" << thrw;
      }
      if (e1 == nullptr) {
        cerr << "Stmt::create(): e1 is null, variable might not be initialized" << thrw;
      }
      //assertq(e0 != nullptr && e1 != nullptr, "create 1");
      ret->m_exp_a = e0;
      ret->m_exp_b = e1;
    break;

    case LOAD_RECEIVE:
      assertq(e0 != nullptr && e1 == nullptr, "create 2");
      ret->m_exp_a = e0;
    break;

    case GATHER_PREFETCH:
      // Nothing to do
    break;

    default:
      if (!ret->dma.address(in_tag, e0)) {
        fatal("create(Expr,Expr): Tag not handled");
      }
    break;
  }

  return ret;
}


Stmt::Ptr Stmt::create_assign(Expr::Ptr lhs, Expr::Ptr rhs) {
  return create(ASSIGN, lhs, rhs);
}


CExpr::Ptr Stmt::if_cond() const {
  assert(tag == IF);
  assert(m_cond.get() != nullptr);
  return m_cond;
}


CExpr::Ptr Stmt::loop_cond() const {
  assert(tag == WHILE);
  assert(m_cond.get() != nullptr);
  return m_cond;
}


///////////////////////////////////////////////////////////////////////////////
// Class Stmt::Array
///////////////////////////////////////////////////////////////////////////////

/**
 * Stmt::dump() can return Stmt::Array;
 * Collect the values in a string before outputting.
 *
 * This is relevant for adding line numbers.
 */
std::string Stmt::Array::dump(bool show_comments) const {
  if (empty()) return "<Empty>";

  std::string ret;

  for (int i = 0; i < (int) size(); i++) {
    ret << (*this)[i]->dump(show_comments) << "\n";
  }

  return ret;
}


std::string Stmts::dump() const {
  if (empty()) return "<Empty>";

	bool do_line_numbers = LibSettings::dump_line_numbers();
  std::string buf = Array::dump(true);
	std::vector<std::string> lines = split(buf, "\n");

	int count = 0;
  std::string ret;
	std::string pre = "#";
	std::string sp = "  ";

  for (int i = 0; i < (int) lines.size(); i++) {
		// Skip empty lines
		if (lines[i].empty()) {
	  	ret << "\n";
			continue;
		}

		// Skip line numbers for lines starting with spaces
		if (lines[i].compare(0, sp.size(), sp) == 0) {
	  	ret << lines[i] << "\n";
			continue;
		}

		// Skip line numbers for comment lines
		if (lines[i].compare(0, pre.size(), pre) == 0) {
	  	ret << lines[i] << "\n";
			continue;
		}

		if (do_line_numbers) {
	  	ret << count << ": ";
		}

	  ret << lines[i] << "\n";
		count++;
  }

  return ret;
}

}  // namespace V3DLib
