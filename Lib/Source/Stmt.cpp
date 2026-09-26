#include "Stmt.h"
#include "Support/basics.h"
#include "Support/Helpers.h"
#include "vc4/DMA/DMA.h"
#include "LibSettings.h"

namespace V3DLib {

using ::operator<<;  // C++ weirdness

namespace {

const char *dump_stmt_tag(Stmt::Tag tag) {
  switch(tag) {
    case Stmt::SEQ:     return "SEQ";
    case Stmt::WHERE:   return "WHERE";
    case Stmt::WHILE:   return "WHILE";
    case Stmt::FOR:     return "FOR";
     // Add other tags here as required

    default:
      cerr << "dump_stmt_tag() unhandled tag: " << tag;
      return "<UNKNOWN>";
  }
}

}  // anon namespace


// ============================================================================
// Class Stmt
// ============================================================================

Stmt::~Stmt() {}

std::string Stmt::dump() const { return disp_intern(0, false); }

std::string Stmt::dump(bool show_comments, int indent) const {
  return disp_intern(indent, show_comments);
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


Expr::Ptr Stmt::lhs() const { return m_exp_a; }
void Stmt::lhs(Expr::Ptr val) { m_exp_a = val; }
Expr::Ptr Stmt::rhs() const { return m_exp_b; }
void Stmt::rhs(Expr::Ptr val) { m_exp_b = val; }


Expr::Ptr Stmt::assign_lhs() const {
  assert(tag == ASSIGN);
  assert(m_exp_a.get() != nullptr);
  return lhs();
}


Expr::Ptr Stmt::assign_rhs() const {
  assert(tag == ASSIGN);
  assert(m_exp_b.get() != nullptr);
  return rhs();
}


Expr::Ptr Stmt::address() {
  assert(tag == LOAD_RECEIVE);
  assert(m_exp_a.get() != nullptr);
  assert(m_exp_b.get() == nullptr);
  return m_exp_a;
}


bool Stmt::check_blocks() const {
  // then and else blocks may not both be empty
  if (m_stmts_a.empty() && m_stmts_b.empty()) {
    return false;
  }

  if (!m_stmts_a.empty()) {
    for (int i = 0; i < (int) m_stmts_a.size(); i++) {
      if (!m_stmts_a[i]) {
        warn << "check_blocks a fails at i: " << i;
        return false;
      }
    }
  }

  if (!m_stmts_b.empty()) {
    for (int i = 0; i < (int) m_stmts_b.size(); i++) {
      if (!m_stmts_b[i]) {
        warn << "check_blocks b fails at i: " << i;
        return false;
      }
    }
  }

  return true;
}


/**
 * @brief Return then block, if any.
 */
Stmt::Array const &Stmt::then_block() const {
  assert(check_blocks());

  return m_stmts_a;
}


bool Stmt::then_block_empty() const {
  return m_stmts_a.empty();
}


bool Stmt::else_block_empty() const {
  return m_stmts_b.empty();
}


Stmt::Array const &Stmt::body() const {
  assertq(tag == SEQ || tag == WHILE || tag == FOR,
    "Body-statement only valid for SEQ, WHILE and FOR");

  if (m_stmts_a.empty() || !m_stmts_b.empty()) {
    warn << "body for tag " << dump_stmt_tag(tag) << " must have then-block and no else-block"
         << thrw;
  }

  assert(check_blocks());  // Probably superfluous
  return m_stmts_a;
}


Stmt::Array const &Stmt::else_block() const {
  assertq(tag == IF || tag == WHERE, "Else-statement only valid for IF and WHERE");
  // where and else stmt may not both be empty
  assert(!m_stmts_a.empty() || !m_stmts_b.empty());

  return m_stmts_b;
}


/**
 * @return true if block successfully added, false otherwise
 */
bool Stmt::then_block(Array const &in_block) {
  if ((tag == Stmt::IF || tag == Stmt::WHERE ) && m_stmts_a.empty()) {
    //warn << "then_block adding in_block:" << in_block.dump();
    m_stmts_a << in_block;
    assert(!m_stmts_a.empty());
    return true;
  } else {
    assert(false);
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
        //warn << "add_block adding then-block for WHERE: " << block.empty();
        then_block(block);
        return true;
      } else if (else_is_empty) {
        //warn << "add_block adding else-block for WHERE: " << block.empty();
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
  assertq(tag == FOR, "Inc-statement only valid for FOR");
  assert(m_stmts_b.empty());  // Only assign once
  m_stmts_b = arr;
}

namespace {

/**
 * **TODO**: move to Helpers.cpp
 */
std::string indent(std::string const &src, int num_spaces) {
  if (src.empty()) return ""; 

  auto tmp = split(src, "\n");

  std::string out;
  for (int i = 0; i < (int) tmp.size(); i++) {
    auto const &line = tmp[i];

    if (!line.empty()) {
      out << tabs(num_spaces) << line;
    }

    out << "\n";

    //if (i < (int) tmp.size() - 1) {
    //  out << "\n";
    //}
  }

  //out << tabs(num_spaces);

  //if (!src.empty()) {
  //  out << "\n" ;
  //}

  return out;
}

} // anon namespace


std::string Stmt::disp_comments(std::string const &line, int seq_depth) const {
  //warn << "Stmt disp_comments";
  auto instr = InstructionComment::emit_comments(line, ";");

  return indent(instr, 2*seq_depth);
}


/**
 * Dump output for current statement.
 *
 * @return Text representation of current statement.
 */
std::string Stmt::disp_intern(int seq_depth, bool show_comments) const {
  std::string ret;

  switch (tag) {
    case NOP: {
      std::string index = rhs()->dump();
      if (index == "Int 1") {
        ret << "NOP";
      } else {
        ret << "NOP(" << index << ")";
      }
    }
    break;

    case SKIP: ret << "SKIP"; break;

    case ASSIGN:
      ret << "ASSIGN " << assign_lhs()->dump() << " = " << assign_rhs()->dump();
    break;

    case SEQ:
      // At time of writing, only reached within DFT unit test [20260924]
      assert(!m_stmts_a.empty());
      assert(m_stmts_b.empty());
      ret << "SEQ\n"
          << m_stmts_a.dump(show_comments, 1);
    break;

    case WHERE:
      assert(m_where_cond.get() != nullptr);
      ret << "WHERE (" << m_where_cond->dump() << ")\n"
          << "THEN\n"
          << then_block().dump(show_comments, /*seq_depth + */ 1);

      if (!else_block_empty()) {
        ret << "\n"
            << "ELSE\n"
            << else_block().dump(show_comments, /* seq_depth + */ 1);
      }
    break;

    case IF:
      assert(m_cond.get() != nullptr);
      ret << "IF (" << m_cond->dump() << ") THEN\n"
          << "  ";

      if (then_block_empty()) {
        ret << "<<NO THEN-BLOCK PRESENT>>";
      } else {
        ret << then_block().dump(show_comments);
      }

      if (!else_block_empty()) {
        ret << "\nELSE\n"
            << "  " << else_block().dump(show_comments);
      }
    break;

    case WHILE:
      assert(m_cond.get() != nullptr);
      ret << "WHILE (" << m_cond->dump() << ")\n";

      if (then_block_empty()) {
        ret << "  <<NO THEN-BLOCK PRESENT>>";
      } else {
        ret << then_block().dump(show_comments, /* seq_depth + */ 1);
      }

      // There is no ELSE for while. TODO: check, code elsewhere says otherwise
    break;

    case GATHER_PREFETCH:  ret << "GATHER_PREFETCH"; break;
    case FOR:              ret << "FOR";             break;
    case LOAD_RECEIVE:     ret << "LOAD_RECEIVE";    break;
    case BARRIER:          ret << "BARRIER";         break;

    default: {
      std::string tmp = DMA::dump_tag(tag);   // Check for unhandled DMA stuff

      if (!tmp.empty()) {                     // DMA
        ret << tmp;
      } else {                                // Unknown tag
        std::string msg;
        msg << "Stmt::disp_intern() " << "Unknown tag '" << tag << "'";

        if (tag < 0 || tag >= NUM_TAGS) {
          msg << "; tag out of range";
        }

        assertq(false, msg);
      }
    }
    break;
  }

  if (show_comments) {
    return disp_comments(ret, seq_depth);
  }

  assert(!ret.empty());
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
      ret->m_exp_a = e0;
      ret->m_exp_b = e1;
    break;

    case NOP:
      assertq(e0 == nullptr && e1 != nullptr, "create NOP");
      ret->m_exp_b = e1;
    break;

    case LOAD_RECEIVE:
      assertq(e0 != nullptr && e1 == nullptr, "create LOAD_RECEIVE");
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
 *
 */
std::string Stmt::Array::dump(bool show_comments, int indent) const {
  if (empty()) return "<Empty>";

  // Count the number of endlines at end of string
  auto ending_newlines = [] (std::string const &s) -> int {
    int count = 0;

    for (int i = (int) s.size() - 1; i >= 0; i--) {
      if (s[i] == '\n') {
        ++count;
      } else {
        break;
      }
    }

    return count;
  };

  std::string ret;

  for (int i = 0; i < (int) size(); i++) {
    // a line can actually be multiple output lines if it contains arrays.
    // Internal arrays have their own newlines, which can result in multiple endlines per line
    auto const &line = *(*this)[i];
    auto tmp = line.dump(show_comments, indent);

    // Replace multiple ending newlines with a single newline
    int nl_count = ending_newlines(tmp);
    if (nl_count > 1) {
      tmp.erase(tmp.length() - (nl_count - 1));
    }

    ret << tmp;
  }

  return ret;
}


Stmt::Array &Stmt::Array::operator<<(Array const &b) {
  auto &a = *this;
  a.insert(a.end(), b.begin(), b.end());
  return *this;
}


/**
 * @brief Ouput a text dump for a Stmt instruction list.
 *
 * This instruction dump is different from the others, because
 * Stmt instances have internal structure, which is indented in the output.
 */
std::string Stmts::dump() const {
  if (empty()) return "<Empty>";

  bool do_line_numbers = LibSettings::dump_line_numbers();
  std::string buf = Array::dump(true);
  std::vector<std::string> lines = split(buf, "\n");

  int count = 0;
  std::string ret;
  std::string pre = ";";
  std::string sp = "  ";

  for (int i = 0; i < (int) lines.size(); i++) {
    auto const &line = lines[i];

    std::string count_str;
    if (do_line_numbers) {
      count_str << count << ": ";
    }

    // Skip empty lines
    if (trim_s(line).empty()) {
      continue;
    }

    // Skip line numbers for comment lines
    // Tests for indented lines starting with comment character
    if (trim_s(line).compare(0, pre.size(), pre) == 0) {
      ret << indentBy((int) count_str.size()) << line << "\n";
      continue;
    }

    ret << count_str << line << "\n";

    count++;
  }

  return ret;
}


/**
 * @return index of last uniform in initial uniforms list
 *
 * This is specific for the Source statements list.
 *
 * Returns -1 in the extremely unlikely event of no uniforms present.
 */
int Stmts::lastUniform(bool do_dump) {
  std::string buf;

  int i = 0;
  for (; i < (int) size(); ++i) {
    auto &item = *at(i);

    if (item.tag != Stmt::ASSIGN) break;

    auto rhs = item.rhs();
    if (rhs == nullptr) break;
    if (rhs->tag() != Expr::VAR) break; 

    Var var = rhs->var();
    if (var.tag() != UNIFORM) break; 

    buf << item.dump(true) << "\n";
  }

  if (do_dump) {
    warn << "lastUniform: \n" << buf;
  }
  return (i - 1);
}

}  // namespace V3DLib
