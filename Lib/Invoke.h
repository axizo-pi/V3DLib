#ifndef _V3DLIB_INVOKE_H_
#define _V3DLIB_INVOKE_H_
#include "Common/Seq.h"
#include "Common/SharedArray.h"

namespace V3DLib {

class BaseKernel;  // Forward declaration

using Params  = IntList; 

class ScheduledJob {
public:
  ScheduledJob(Code const &in_code, Params const &in_params): code(in_code), params(in_params) {}

  int num_params() const;

  Code   const &code;
  Params const &params;
  int           params_offset = -1;
};


class ScheduledJobs : public std::vector<ScheduledJob> {
public:
  int num_params() const;
};


namespace Invoke {

void schedule(BaseKernel const &kernel);
void run();

}  // namespace Invoke
}  // namespace V3DLib

#endif  // _V3DLIB_INVOKE_H_
