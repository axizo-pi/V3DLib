#include "CompileData.h"

#ifdef OUTPUT_COMPILEDATA

#include "Support/basics.h"

namespace V3DLib {

//using ::operator<<;  // C++ weirdness

CompileData compile_data;

std::string CompileData::dump() const {
	std::string register_blurb;
	register_blurb
		  << "Line Layout:\n"
		  << "\n"
		  << "  l: r;src(first, last, count);dst: {list}; live(first, last, count)\n"
		  << "\n"
		  << "       l: Index of variable in Target code.\n"
			<< "       r: Index of assigned register, '_' if not assigned.\n"
			<< "     src: Range of line numbers where variable is used as source,\n"
		  << "          and number of times it is used as source in that range;\n"
		  << "          'src(none)' if not used as source.\n"
			<< "     dst: List of line numbers where variable is used as destination, may be empty.\n"
			<< "    live: Range of line numbers where variable is live and line count;\n"
		  << "          'live(none)' if no range.\n"
		  << "\n";

  std::string ret;
  ret << title("Liveness dump")
		  << " - Index is line number in target code, see below.\n"
			<< " - Value between brackets is number of variables live at that line.\n"
			<< " - Values in list are the indexes of the live variables.\n"
		  << "\n"
      << liveness_dump
/*
      << title("Register Usage Dump")
      << reg_usage_dump
*/
      << title("Allocated Registers to Variables")
			<< register_blurb
      << allocated_registers_dump;
/*
  if (!target_code_before_optimization.empty()) {
    ret << title("Target code before optimization")
        << target_code_before_optimization;
  }

  if (!target_code_before_regalloc.empty()) {
    ret << title("Target code before regAlloc()")
        << target_code_before_regalloc;
  }
*/

  if (!target_code_before_liveness.empty()) {
    ret << title("Target Code")
        << " - Before liveness, after peepholes.\n"
				<< "\n"
        << target_code_before_liveness;
  }

  return ret;
}


void CompileData::clear() {
  liveness_dump.clear();
  target_code_before_optimization.clear();
  target_code_before_regalloc.clear();
  target_code_before_liveness.clear();
  allocated_registers_dump.clear();
  num_accs_introduced = 0;
  num_instructions_combined = 0;
}

}  // namespace V3DLib

#endif // OUTPUT_COMPILEDATA
