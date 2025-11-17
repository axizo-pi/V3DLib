#ifndef _V3DLIB_V3D_INSTR_INSTR_H
#define _V3DLIB_V3D_INSTR_INSTR_H
#include <cstdint>
#include <string>
#include <vector>
#include "v3d_api.h"
#include "Support/InstructionComment.h"
#include "Source.h"
#include "Encode.h"
#include "Target/instr/ALUInstruction.h"
#include "BaseSource.h"


namespace V3DLib {

class ALUInstruction;

namespace v3d {

class RegSet {
public:	
	RegSet(int count);

	unsigned size() const { return (unsigned) m_set.size(); }

	void set(unsigned index);
	void clear(unsigned index);
	void add(RegSet const &rhs);
	std::string dump() const;
	bool empty() const;
	unsigned first_filled() const;
	unsigned first_empty() const;

private:
	std::vector<bool> m_set;
};


class ACCSet :public RegSet {
public:	
	ACCSet() : RegSet(6) {}
};


class RFSet :public RegSet {
public:	
	RFSet() : RegSet(64) {}
};


namespace instr {

using rf = RFAddress;
using si = SmallImm;


/**
 * NOTE: branch condition is distinct from add/mul alu assign condition tags.
 *       Would it then be possible to combine them?? E.g. something like:
 * 
 *           bb(L0).ifna().a0();
 *           bb(L0).nop().ifna().a0();  // Would use mul alu flag
 *       
 *       TODO examine this, not expecting it to work but better to know for sure
 */
class Instr : public v3d_qpu_instr, public InstructionComment {

  // Label support
private:
  bool m_is_label = false;
  int  m_label    = -1;     // Also use for branch with label

public:
  bool is_label()        const { return m_is_label; }
  int  label()           const { assert(m_is_label); return m_label; }
  bool is_branch_label() const;
  int  branch_label()    const;

  void is_label(bool val) { m_is_label = val; }
  void label(int val);
  void label_to_target(int offset);
  // End Label support

public:
  Instr(uint64_t in_code = NOP());

  bool skip() const { return m_skip; }
  void skip(bool val) { m_skip = val; }

  // Grumbl
  Instr &header(std::string const &msg) { InstructionComment::header(msg);  return *this; }
  Instr &comment(std::string msg)       { InstructionComment::comment(msg); return *this; }
  std::string const &header() const     { return InstructionComment::header();}
  std::string const &comment() const    { return InstructionComment::comment();}

  bool is_branch()  const;
  bool has_signal(bool all_signals = false) const;
  bool flag_set() const;
  void set_cond_tag(AssignCond cond);
  void set_push_tag(SetCond set_cond);

  std::string dump() const; 
  std::string mnemonic(bool with_comments = false) const;
  uint64_t bytecode() const;
  static std::string dump(uint64_t in_code);
  static std::string mnemonic(uint64_t in_code);
  static std::string mnemonics(std::vector<uint64_t> const &in_code);

  operator uint64_t() const { return bytecode(); }

  void set_branch_condition(V3DLib::BranchCond src_cond);

  bool add_nop()    const { return alu.add.op == V3D_QPU_A_NOP; }
  bool mul_nop()    const { return alu.mul.op == V3D_QPU_M_NOP; }
  bool is_nop()     const { return add_nop() && mul_nop(); }  // This does not include signals having sig_addr set 
  bool add_nocond() const { return flags.ac == V3D_QPU_COND_NONE; }
  bool mul_nocond() const { return flags.mc == V3D_QPU_COND_NONE; }


  static bool compare_codes(uint64_t code1, uint64_t code2);

  bool check_dst() const;
  bool uses_sig_dst() const;
  bool is_ldtmu() const { assert(sig_dst_count() <= 1); return sig.ldtmu; }
  DestReg sig_dest() const;
  DestReg add_dest() const;
  DestReg mul_dest() const;

private:
  DestReg add_src_dest(v3d_qpu_input src) const;
  
public:
  DestReg add_src_a() const;
  DestReg add_src_b() const;
  DestReg mul_src_a() const;
  DestReg mul_src_b() const;

  bool is_src(DestReg const &dst_reg) const;
  bool is_dst(DestReg const &dst_reg) const;

  void alu_add_dst(Location const &dst); // public because needed in Mnemonics
  bool alu_add_a(Source const &src);     // idem

  bool alu_add_set(Location const &dst, Source const &a, Source const &b);
  bool alu_mul_set(Location const &dst, Source const &a);
  bool alu_mul_set(Location const &dst, Source const &a, Source const &b);

  bool alu_add_set(V3DLib::Instr const &src_instr);
  bool alu_mul_set(V3DLib::Instr const &src_instr);

  bool alu_set(V3DLib::Instr const &src_instr);

	// BaseSource implementation
  void alu_mul_dst(Location const &dst);
  void alu_mul_a(BaseSource const &src);
  void alu_mul_b(BaseSource const &src);
	bool alu_mul_a_safe(BaseSource const &src);
	bool alu_mul_b_safe(BaseSource const &src);

private:
  bool alu_add_set_b(Source const &src);
  bool alu_mul_a(Source const &src);
  bool alu_mul_set_b(Source const &src);

  std::unique_ptr<Source> alu_src(v3d_qpu_mux src) const;  // < vc7

public:
  // TODO see if this can be cleaned up;  could be replaced by DestReg, but I prefer BaseSource
  std::unique_ptr<Location> add_alu_dst() const;
  std::unique_ptr<Location> mul_alu_dst() const;
  std::unique_ptr<Source> add_alu_a() const;
  std::unique_ptr<Source> add_alu_b() const;
  std::unique_ptr<Source> mul_alu_a() const;
  std::unique_ptr<Source> mul_alu_b() const;

  BaseSource alu_add_dst() const;
  BaseSource alu_add_a() const;
  BaseSource alu_add_b() const;
  BaseSource alu_mul_a() const;
  BaseSource alu_mul_b() const;

protected:
  static uint64_t NOP();

  void init(uint64_t in_code);
  void set_branch_condition(v3d_qpu_branch_cond cond);

  void set_sig_addr(Location const &loc);

private:
  bool m_skip = false;

  std::string pretty_instr() const;
  int sig_dst_count() const;

  enum CheckSrc {
    CHECK_ADD_A,
    CHECK_ADD_B,
    CHECK_MUL_A,
    CHECK_MUL_B,
  };

  bool raddr_in_use(CheckSrc check_src, v3d_qpu_mux mux) const;
  bool raddr_a_is_safe(Location const &loc, CheckSrc check_src) const;
  bool raddr_b_is_safe(Location const &loc, CheckSrc check_src) const;

	bool alu_set_src(Source const &src, v3d_qpu_input &input, CheckSrc check_src);
	bool has_small_imm() const;

//
// vc7: support for converting acc's to rf'
//	
public:
	bool uses_acc() const;
	ACCSet acc_usage() const;
	RFSet  rf_usage() const;
	int replace_acc_with_rf(unsigned acc, unsigned rf);

private:
	bool add_a_is_acc() const;
	bool add_b_is_acc() const;
	bool add_dst_is_acc() const;
	bool mul_a_is_acc() const;
	bool mul_b_is_acc() const;
	bool mul_dst_is_acc() const;
	bool sig_is_acc() const;

	bool m_add_a_is_reg   = false;
	bool m_add_b_is_reg   = false;
	bool m_add_dst_is_reg = false;
	bool m_mul_a_is_reg   = false;
	bool m_mul_b_is_reg   = false;
	bool m_mul_dst_is_reg = false;
	// sig_addr doesn't need a special bool, sig_magic does the job
};

}  // instr


using ByteCode = std::vector<uint64_t>;  // For better understanding

class Instructions : public std::vector<instr::Instr> {
  using Parent = std::vector<instr::Instr>;

public:
  Instructions() = default;
  Instructions(Parent const &rhs) : Parent(rhs) {}

  Instructions &header(std::string const &msg) { front().header(msg);  return *this; }
  Instructions &comment(std::string msg, bool to_front = true);
  void set_cond_tag(AssignCond cond);
  bool check_consistent() const;

	bool uses_acc() const;
	ACCSet acc_usage() const;
	RFSet  rf_usage() const;
	int replace_acc_with_rf(unsigned acc, unsigned rf);

	ByteCode bytecode() const;
	std::string dump() const;
};


/**
 * Why this has suddenly become necessary is beyond me.
 *
 * Just going with the flow here, some C++ shit is arcane.
 */
inline Instructions &operator<<(Instructions &lhs, instr::Instr const &rhs) {
  lhs.push_back(rhs);
  return lhs;
}


inline Instructions &operator<<(Instructions &lhs, Instructions const &rhs) {
  for (auto const &item : rhs) {
    lhs << item;
  }

  return lhs;
}

}  // v3d
}  // V3DLib

#endif  // _V3DLIB_V3D_INSTR_INSTR_H
