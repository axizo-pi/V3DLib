#include "UseDef.h"
#include "Support/basics.h"

namespace V3DLib {

using namespace Target;

///////////////////////////////////////////////////////////////////////////////
// Class UseDefReg
///////////////////////////////////////////////////////////////////////////////

std::string UseDefReg::dump() const {
  std::string ret;

  ret << "(def: ";
  if (def.tag != NONE) {
    ret << def.dump() << ", ";
  }
  ret << "; ";

  ret << "use: ";
  for (auto const &reg : use) {
    ret << reg.dump() << ", ";
  }
  ret << ") ";

  return ret;
}


UseDefReg::UseDefReg(Instr const &instr, bool set_use_where) :
  use(instr.src_regs(set_use_where)),
  def(instr.dst_reg())
 {}


///////////////////////////////////////////////////////////////////////////////
// Class UseDef
///////////////////////////////////////////////////////////////////////////////

std::string UseDef::dump() const {
  std::string ret;

  ret << "(def: ";

  if (def.tag != NONE) {
   ret << def.dump();
  }

  ret << "; " 
      <<  "use: " << use.dump() << ") "; 

  return ret;
}


UseDef::UseDef(Target::Instr const &instr, bool do_accumulators, bool set_use_where) :
  def(NONE, 0)
{
  if (do_accumulators) {
    // Param `set_use_where` can be ignored, dst always added

    uint32_t acc_mask = instr.get_acc_usage();  // This includes dst in mask

    for (int i = 0; i < 6; ++i) {
      bool is_set = (acc_mask & (1 << i)) != 0;

      if (is_set) {
        use.insert(i);
      }
    }

    auto dst = instr.dst_reg();
    if (dst.tag == ACC) {
      //warn << "dst: " << dst.dump();
      def = dst;
    }

/*
    if (use.size() > 0) {
      warn << "UseDef: " << dump() << ", instr: " << instr.mnemonic();
    }
*/
  } else {
    // Expecting registers only in regfile A here.
    bool no_b = (
         (instr.dst_reg().tag   != REG_B)
      && (instr.src_a_reg().tag != REG_B)
      && (instr.src_b_reg().tag != REG_B)
    );

    if (!no_b) {
      warn << "no_b fail: " << instr.dump();
      //warn << "reg_a: " << instr.src_a_reg().dump();
      //warn << "reg_b: " << instr.src_b_reg().dump();

      assertq(false, "no_b fail");
    }

    use = instr.src_a_regs(set_use_where);
    def = instr.dst_a_reg();
  }
}

}  // namespace V3DLib
