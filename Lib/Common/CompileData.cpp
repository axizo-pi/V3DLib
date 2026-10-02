#include "CompileData.h"

#ifdef OUTPUT_COMPILEDATA

#include "Support/basics.h"

namespace V3DLib {

//using ::operator<<;  // C++ weirdness

CompileData compile_data;

std::string CompileData::dump() const {
  std::string register_blurb =
      "- Line Layout:\n"
      "\n"
      "  l: r;src(first, last, count);dst: {list}; live(first, last, count)\n"
      "\n"
      "        l: Index of variable in Target code.\n"
      "        r: Index of assigned register, '_' if not assigned.\n"
      "src_range: Range of line numbers where variable is used as source,\n"
      "           and number of times it is used as source in that range;\n"
      "           'src(none)' if not used as source.\n"
      "      src: List of line numbers where variable is used as source.\n"
      "      dst: List of line numbers where variable is used as destination.\n"
      "     live: Range of line numbers where variable is live and line count;\n"
      "           'live(none)' if no range.\n"
      "\n"
      "- Missing variable indexes or indexes flagged as 'Not used' are most likely "
        "replaced by accumulators.\n"
      "\n";

  std::string ret;
  ret << title("Liveness dump")
      << "List of indexes of available accumulators per line.\n"
         "\n"
         " - Index is line number in target code, see below.\n"
         " - Value between brackets is number of accumulators live at that line.\n"
         " - Values in list are the indexes of the live accumulators.\n"
         "\n"
      << liveness_dump
/*
      << title("Register Usage Dump")
      << reg_usage_dump
*/
      << title("Allocated Registers to Variables")
      << register_blurb
      << allocated_registers_dump;

  if (!target_code_after_immediates.empty()) {
    ret << title("Target Code after Immediates")
        << target_code_after_immediates;
  }

/*
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

  if (!target_code_after_regalloc.empty()) {
    ret << title("Target code after regAlloc()")
        << target_code_after_regalloc;
  }

  return ret;
}


void CompileData::clear() {
  liveness_dump.clear();
  target_code_after_immediates.clear();
  target_code_before_regalloc.clear();
  target_code_before_liveness.clear();
  allocated_registers_dump.clear();
  num_accs_introduced = 0;
  num_instructions_combined = 0;
}

}  // namespace V3DLib

#endif // OUTPUT_COMPILEDATA
