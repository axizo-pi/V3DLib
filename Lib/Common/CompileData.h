#ifndef _V3DLIB_COMMON_COMPILEDATA_H_
#define _V3DLIB_COMMON_COMPILEDATA_H_

#include "defines.h"

#ifdef OUTPUT_COMPILEDATA

#include "Target/instr/Reg.h"
#include <string>
#include <vector>

namespace V3DLib {

struct CompileData {
  std::string liveness_dump;
  std::string target_code_after_immediates;
  std::string target_code_before_regalloc;
  std::string target_code_after_regalloc;
  std::string target_code_before_liveness;
  std::string allocated_registers_dump;
  std::string reg_usage_dump;
  int num_accs_introduced = 0;
  int num_instructions_combined = 0;

  std::string dump() const;
  void clear();
};

extern CompileData compile_data;

}  // namespace V3DLib

#endif // OUTPUT_COMPILEDATA

#endif  // _V3DLIB_COMMON_COMPILEDATA_H_
