#include "Conditions.h"
#include "Support/basics.h"
#include "Support/Platform.h"
#include "Source/BExpr.h"

namespace V3DLib {
namespace {

/**
 * Negate a condition flag
 */
Flag negFlag(Flag flag) {
  switch(flag) {
    case ZS: return ZC;
    case ZC: return ZS;
    case NS: return NC;
    case NC: return NS;
  }

  // Not reachable
  assert(false);
  return ZS;
}


const char *dump_flag(Flag flag) {
  switch (flag) {
    case ZS: return "ZS";
    case ZC: return "ZC";
    case NS: return "NS";
    case NC: return "NC";
    default: assert(false); return "";
  }
}

} // anon namespace

///////////////////////////////////////////////////////////////////////////////
// Class BranchCond
///////////////////////////////////////////////////////////////////////////////

BranchCond BranchCond::negate() const {
  BranchCond ret;

  switch (tag) {
    case COND_NEVER:  ret.tag  = COND_ALWAYS; break;
    case COND_ALWAYS: ret.tag  = COND_NEVER;  break;
    case COND_ANY:    ret.tag  = COND_ALL; ret.flag = negFlag(flag); break;
    case COND_ALL:    ret.tag  = COND_ANY; ret.flag = negFlag(flag); break;
    default:
      assert(false);
      break;
  }

  return ret;
}


uint8_t BranchCond::encode() const {
  assertq(Platform::compiling_for_vc4(), "BranchCond::encode(): this call is vc4 only");

  switch (tag) {
    case COND_NEVER:
      fatal("V3DLib: 'never' condition not supported");
    case COND_ALWAYS: return 15;
    case COND_ALL:
      switch (flag) {
        case ZS: return 0;
        case ZC: return 1;
        case NS: return 4;
        case NC: return 5;
        default: break;
      }
    case COND_ANY:
      switch (flag) {
        case ZS: return 2;
        case ZC: return 3;
        case NS: return 6;
        case NC: return 7;
        default: break;
      }

    default:
      fatal("BranchCond::encode(): missing case");
      return 0;
  }
}


std::string BranchCond::dump() const {
  std::string ret;

  switch (tag) {
    case COND_ALL:
      ret << "all(" << dump_flag(flag) << ")";
      break;
    case COND_ANY:
      ret << "any(" << dump_flag(flag) << ")";
      break;
    case COND_ALWAYS:
      ret = "always";
      break;
    case COND_NEVER:
      ret = "never";
      break;
  }

  return ret;
}


///////////////////////////////////////////////////////////////////////////////
// Class SetCond
///////////////////////////////////////////////////////////////////////////////

/*
SetCond::SetCond(CmpOp const &cmp_op) {
  setOp(cmp_op);
}
*/


const char *SetCond::to_string() const {
  switch (m_tag) {
    case NO_COND: return "None";
    case Z:       return "Z";
    case N:       return "N";
    case C:       return "C";
    default:
      assert(false);
      return "<UNKNOWN>";
  }
}


std::string SetCond::dump() const {
  std::string ret;
  if (flags_set()) {
    ret << "{sf-" << to_string() << "}";
  }
  return ret;
}


void SetCond::setFlag(Flag flag) {
  Tag set_tag = NO_COND;

  switch (flag) {
    case ZS: 
    case ZC: 
      set_tag = SetCond::Z;
      break;
    case NS: 
    case NC: 
      set_tag = SetCond::N;
      break;
    default:
      assert(false);  // Not expecting anything else right now
      break;
  }

  tag(set_tag);
}


///////////////////////////////////////////////////////////////////////////////
// Class AssignCond
///////////////////////////////////////////////////////////////////////////////


AssignCond::AssignCond(CmpOp const &cmp_op) :
  m_tag(FLAG),
  m_flag(cmp_op.assign_flag()),
  m_type(cmp_op.type())
{
  assert(m_type == INT32 || m_type == FLOAT);
}


AssignCond::AssignCond(Tag in_tag, Flag in_flag) : m_tag(in_tag), m_flag(in_flag) {}


bool AssignCond::operator==(AssignCond rhs) const {
  return (m_tag == rhs.m_tag && m_flag == rhs.m_flag);
}



AssignCond AssignCond::negate() const {
  AssignCond ret = *this;

  switch (m_tag) {
    case NEVER:  ret.m_tag  = ALWAYS; break;
    case ALWAYS: ret.m_tag  = NEVER;  break;
    case FLAG:   ret.m_flag = negFlag(m_flag); break;
    default:
      assert(false);
      break;
  }

  return ret;
}


std::string AssignCond::dump() const {
  using Tag = AssignCond::Tag;

  // 'int always' is taken as default
  if (!is_float() && m_tag == Tag::ALWAYS) {
    return "";
  }

  std::string ret;
  ret << (is_float()?"float ":"int ");

  switch (m_tag) {
    case Tag::ALWAYS: ret << "always";                      break;
    case Tag::NEVER:  ret << "never";                       break;
    case Tag::FLAG:   ret << "where " << dump_flag(flag()); break;
    default: assert(false);
  }

  if (!ret.empty()) {
    auto tmp = ret;
    ret.clear();
    ret << "(" << tmp << ") ";
  }

  return ret;
}


uint8_t AssignCond::encode() const {
  assertq(Platform::compiling_for_vc4(), "AssignCond::encode(): this call is vc4 only");

  switch (m_tag) {
    case NEVER:  return 0;
    case ALWAYS: return 1;
    case FLAG:
      switch (m_flag) {
        case ZS: return 2;
        case ZC: return 3;
        case NS: return 4;
        case NC: return 5;
     }

    default:
      fatal("AssignCond::encode(): missing case in");
      return 0;
  }
}


/**
 * Translate an AssignCond instance to a BranchCond instance
 *
 * @param do_all  if true, set BranchCond tag to ALL, otherwise set to ANY
 */
BranchCond AssignCond::to_branch_cond(bool do_all) const {
  BranchCond bcond;

  if (is_always()) { bcond.tag = BranchCond::COND_ALWAYS; return bcond; }
  if (is_never())  { bcond.tag = BranchCond::COND_NEVER; return bcond; }

  assert(tag() == AssignCond::FLAG);
 
   bcond.flag = m_flag;
 
   if (do_all) {
    bcond.tag = BranchCond::COND_ALL;
   } else {
    bcond.tag = BranchCond::COND_ANY;
   }

  return bcond;
}

AssignCond always(AssignCond::Tag::ALWAYS);  // Is a global to reduce eyestrain in gdb
AssignCond never(AssignCond::Tag::NEVER);    // idem

}  // namespace V3DLib
