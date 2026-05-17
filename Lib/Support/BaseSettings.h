#ifndef _V3DLIB_SUPPORT_BASESETTINGS_H
#define _V3DLIB_SUPPORT_BASESETTINGS_H
#include <string>

namespace V3DLib {

class BaseKernel;

enum RunType {
  QPU,
  Interpreter,
  Emulator,
  Debugger
};

struct BaseSettings {
  std::string name;

  bool    compile_only = false;
  RunType run_type     = QPU;
  int     num_qpus     = 1;
  bool    output_code  = false;
  bool    show_perf_counters = false;

  void startPerfCounters();
  void stopPerfCounters();
  void dump_code(BaseKernel &k);
  void setMaxQPUs();

private:
  int output_count = 0;
};


}  // namespace V3DLib


#endif // _V3DLIB_SUPPORT_BASESETTINGS_H
