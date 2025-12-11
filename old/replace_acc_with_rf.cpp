
namespace V3DLib {
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


///////////////////////////////////
// Class RegSet
///////////////////////////////////

RegSet::RegSet(int count) {
	m_set.resize(count);

	for (unsigned i = 0; i < m_set.size(); ++i) {
		m_set[i] = false;
	}
}


void RegSet::set(unsigned index) {
	assert(index < m_set.size());
	m_set[index] = true;
}


void RegSet::clear(unsigned index) {
	assert(index < m_set.size());
	m_set[index] = false;
}


void RegSet::add(RegSet const &rhs) {
	for (unsigned i = 0; i < rhs.m_set.size(); ++i ) {
		if (rhs.m_set[i]) m_set[i] = true;
	}
}

std::string RegSet::dump() const {
	std::string ret;

	for (unsigned i = 0; i < m_set.size(); ++i ) {
		if (m_set[i]) {
			ret += "1";
		} else {
			ret += "0";
		}
	}

	return ret;
}


bool RegSet::empty() const {
	for (unsigned i = 0; i < m_set.size(); ++i ) {
		if (m_set[i]) return false;
	}

	return true;
}


unsigned RegSet::first_filled() const {
	for (unsigned i = 0; i < m_set.size(); ++i ) {
		if (m_set[i]) return i;
	}

	Log::error << "first_filled: no values present" << Log::thrw;

	return (unsigned) -1;
}


unsigned RegSet::first_empty() const {
	for (unsigned i = 0; i < m_set.size(); ++i ) {
		if (!m_set[i]) return i;
	}

	Log::error << "first_empty: no values available" << Log::thrw;

	return (unsigned) -1;
}

namespace instr {

class Instr : public v3d_qpu_instr, public InstructionComment {
public
	ACCSet acc_usage() const;
	RFSet  rf_usage() const;

	//
	// vc7: support for converting acc's to rf'
	//	
	void replace_acc_with_rf();

	bool uses_acc() const;

private:
	// Need to be set when a reg is filled in
	bool m_add_a_is_reg   = false;
	bool m_add_b_is_reg   = false;
	bool m_add_dst_is_reg = false;
	bool m_mul_a_is_reg   = false;
	bool m_mul_b_is_reg   = false;
	bool m_mul_dst_is_reg = false;
	// sig_addr doesn't need a special bool, sig_magic does the job

	bool add_a_is_acc() const;
	bool add_b_is_acc() const;
	bool add_dst_is_acc() const;
	bool mul_a_is_acc() const;
	bool mul_b_is_acc() const;
	bool mul_dst_is_acc() const;
	bool sig_is_acc() const;

	int replace_acc_with_rf(unsigned acc, unsigned rf);

	//...
};


namespace {

/**
 * The primary goal of this functions is to detect any other sig's 
 * that could possibly use sig_addres on vc7.
 *
 * Known sig's are left out
 */
bool unknown_sig_is_set(struct v3d_qpu_sig const &sig) {
	return
    //sig.ldunif ||    - uses r5 implitly on vc6
    sig.ldunifa ||
    sig.ldunifarf ||
    sig.ldtmu ||
    sig.ldvary ||
    sig.ldvpm ||
    sig.ldtlb ||
    sig.ldtlbu ||
    sig.ucb ||
    sig.rotate ||
    sig.wrtmuc
	;
}

} // anon namespace


////////////////////////////////////////
// Class Instr
////////////////////////////////////////

ACCSet Instr::acc_usage() const {
	ACCSet ret;

	if (add_a_is_acc()) ret.set(alu.add.a.raddr);  // You would expect mux here
	if (add_b_is_acc()) ret.set(alu.add.b.raddr);

	if (add_dst_is_acc()) {
		assert(alu.add.magic_write);
		ret.set(alu.add.waddr);
	}

	if (mul_a_is_acc()) ret.set(alu.mul.a.raddr);  // You would expect mux here
	if (mul_b_is_acc()) ret.set(alu.mul.b.raddr);  // You would expect mux here

	if (mul_dst_is_acc()) {
		assert(alu.mul.magic_write);
		ret.set(alu.mul.waddr);
	}

	if (sig_is_acc()) ret.set(sig_addr);

	return ret;
}


RFSet Instr::rf_usage() const {
	RFSet ret;

  if (                                 // This is a list of opcodes which do not take add inputs
		alu.add.op != V3D_QPU_A_NOP  && 
		alu.add.op != V3D_QPU_A_EIDX &&
		alu.add.op != V3D_QPU_A_TMUWT
	) {
		if (!m_add_a_is_reg && !sig.small_imm_a) {
			ret.set(alu.add.a.raddr);
		}

	  if (                                 // This is a list of opcodes which take only add.a input
			alu.add.op != V3D_QPU_A_MOV 
		) {
			if (!m_add_b_is_reg && !sig.small_imm_b) {
				/*
				if (alu.add.b.raddr == 0) {
					breakpoint
				}
				*/
				ret.set(alu.add.b.raddr);
			}
		}
	}

  if (alu.add.op != V3D_QPU_A_NOP) {
		if (!m_add_dst_is_reg) {
			assert(!alu.add.magic_write);
	 		ret.set(alu.add.waddr);
		}
	}

  if (alu.mul.op != V3D_QPU_M_NOP) {
		if (!m_mul_a_is_reg && !sig.small_imm_c) ret.set(alu.mul.a.raddr);
		if (!m_mul_b_is_reg && !sig.small_imm_d) ret.set(alu.mul.b.raddr);

		if (!m_mul_dst_is_reg) {
			assert(!alu.mul.magic_write);
	 		ret.set(alu.mul.waddr);
		}
	}

	// This depends on setting the sig values; prob need to check on all the sig's
	if (!sig_magic) {
		if (unknown_sig_is_set(sig)) {
			Log::warn << "Some unknown sig is set; check it out";
			breakpoint;
		}

		if (sig.ldunifrf) {
			ret.set(sig_addr);
		}
	}
	

	return ret;
}


int Instr::replace_acc_with_rf(unsigned acc, unsigned rf) {
	int count = 0;

	if (m_add_a_is_reg && alu.add.a.raddr == acc) {
		alu.add.a.raddr = (uint8_t) rf;
		count++;
		m_add_a_is_reg = false;
	}

	if (m_add_b_is_reg && alu.add.b.raddr == acc) {
		alu.add.b.raddr = (uint8_t) rf;
		count++;
		m_add_b_is_reg = false;
	}

	if (m_add_dst_is_reg && alu.add.waddr == acc) {
		assert(alu.add.magic_write);
		alu.add.magic_write = false;
		alu.add.waddr = (uint8_t) rf;
		count++;
		m_add_dst_is_reg = false;
	}

	if (m_mul_a_is_reg && alu.mul.a.raddr == acc) {
		alu.mul.a.raddr = (uint8_t) rf;
		count++;
		m_mul_a_is_reg = false;
	}

	if (m_mul_b_is_reg && alu.mul.b.raddr == acc) {
		alu.mul.b.raddr = (uint8_t) rf;
		count++;
		m_mul_b_is_reg = false;
	}

	if (m_mul_dst_is_reg && alu.mul.waddr == acc) {
		assert(alu.mul.magic_write);
		alu.mul.magic_write = false;
		alu.mul.waddr = (uint8_t) rf;
		count++;
		m_mul_dst_is_reg = false;
	}

	if (sig_magic && sig_addr == acc) {
		sig_addr = (uint8_t) rf;
		sig_magic = false;
		count++;
	}

	return count;
}


/**
 * Replace acc's with rf addresses.
 *
 * The idea is to adjust vc6 instruction code to run
 * on vc7. It works fine, but has limited usage.
 *
 * This was not as useful as I hoped; there is more stuff
 * to do for vc6->vc7 conversion.
 * Accumulators is only part of the conversion, many 
 * commands are different (eg. sin).
 *
 * @return Number of address locations that were converted
 */
void Instructions::replace_acc_with_rf() {

	if (V3DLib::Platform::compiling_for_vc7() && uses_acc()) {
		auto acc = acc_usage();
		auto rf  = rf_usage();

		Log::debug 
			<< "\n"
			<< "  ret uses acc's : "  << acc.dump()       << "\n"
			<< "  ret uses rf's  : "  << rf.dump()        << "\n"
			<< "  First available: "  << rf.first_empty() << "\n"
		;

		int replaced = 0;
		while (!acc.empty()) {
			replaced += replace_acc_with_rf(acc.first_filled(), rf.first_empty());
			acc.clear(acc.first_filled());
			rf.set(rf.first_empty());
		}

		Log::debug << "# replaced reg's: "  << replaced << "\n";

		acc = acc_usage();
		rf  = rf_usage();

		Log::debug 
			<< "\n"
			<< "  acc after replace: "  << acc.dump()       << "\n"
			<< "  ret after replace: "  << rf.dump()       << "\n"
		;
	}
}


bool Instr::uses_acc() const {
	return
		add_a_is_acc() ||
		add_b_is_acc() ||
		add_dst_is_acc() ||
		mul_a_is_acc() ||
		mul_b_is_acc() ||
		mul_dst_is_acc() ||
		sig_is_acc()
	;
}


bool Instr::add_a_is_acc()   const { return (m_add_a_is_reg && alu.add.a.raddr <= V3D_QPU_WADDR_R5); }
bool Instr::add_b_is_acc()   const { return (m_add_b_is_reg && alu.add.b.raddr <= V3D_QPU_WADDR_R5); }
bool Instr::add_dst_is_acc() const { return (m_add_dst_is_reg && alu.add.waddr <= V3D_QPU_WADDR_R5); }
bool Instr::mul_a_is_acc()   const { return (m_mul_a_is_reg && alu.mul.a.raddr <= V3D_QPU_WADDR_R5); }
bool Instr::mul_b_is_acc()   const { return (m_mul_b_is_reg && alu.mul.b.raddr <= V3D_QPU_WADDR_R5); }
bool Instr::mul_dst_is_acc() const { return (m_mul_dst_is_reg && alu.mul.waddr <= V3D_QPU_WADDR_R5); }
bool Instr::sig_is_acc()     const { return (sig_magic && sig_addr <= V3D_QPU_WADDR_R5); }


std::string Instr::mnemonic(bool with_comments) const {
// ...

	if (with_comments && Platform::compiling_for_vc7()) {
		std::string buf;

		if (add_dst_is_acc()) buf << "add.waddr ";
		if (add_a_is_acc())   buf << "add.a ";
		if (add_b_is_acc())   buf << "add.b ";
		if (mul_dst_is_acc()) buf << "mul.waddr ";
		if (mul_a_is_acc())   buf << "mul.a ";
		if (mul_b_is_acc())   buf << "mul.b ";
		if (sig_is_acc())     buf << "sig_addr ";

		if (!buf.empty()) {
			int spaces = 80 - (int) ret.size();
			for (int i = 0; i < spaces; ++i) ret << " ";

			ret << "# Acc's: " << buf;
		}

	}
	
// ...
}


////////////////////////////////////////
// Class Instructions
////////////////////////////////////////

bool Instructions::uses_acc() const {
  for (auto &instr : *this) {
    if (instr.uses_acc()) return true;
	}

	return false;
}


ACCSet Instructions::acc_usage() const {
	ACCSet ret;

  for (auto &instr : *this) {
		ret.add(instr.acc_usage());
	}

	return ret;
}


RFSet Instructions::rf_usage() const {
	RFSet ret;

  for (auto &instr : *this) {
		ret.add(instr.rf_usage());
	}

	return ret;
}


/**
 * @return number of src/dst values with acc's replaced with  rf's
 */
int Instructions::replace_acc_with_rf(unsigned acc, unsigned rf) {
	int count = 0;

	Log::debug << "replace_acc_with_rf: replacing acc" << acc << " with rf" << rf;

  for (auto &instr : *this) {
		count += instr.replace_acc_with_rf(acc, rf);
	}

	return count;
}

}  // instr
}  // v3d
}  // V3DLib


