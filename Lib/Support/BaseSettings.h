#ifndef _V3DLIB_SUPPORT_BASESETTINGS_H
#define _V3DLIB_SUPPORT_BASESETTINGS_H
#include <string>

namespace V3DLib {

class BaseKernel;

struct BaseSettings {
  std::string name;

  bool compile_only = false;
  int  run_type     = 0;
  int  num_qpus     = 1;
  bool output_code  = false;
#ifdef QPU_MODE
  bool   show_perf_counters = false;
#endif  // QPU_MODE

  void startPerfCounters();
  void stopPerfCounters();
  void process(BaseKernel &k);

private:
  int output_count = 0;
};


}  // namespace V3DLib


#endif // _V3DLIB_SUPPORT_BASESETTINGS_H
