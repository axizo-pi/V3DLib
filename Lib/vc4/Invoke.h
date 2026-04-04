#ifndef _V3DLIB_VC4_INVOKE_H_
#define _V3DLIB_VC4_INVOKE_H_
#include <stdint.h>
#include "Common/Seq.h"
#include "Common/SharedArray.h"

namespace V3DLib {

/**
 * @brief Mixin class for Mailbox invocation
 *
 * Prepares the data for the call and executes the call.
 */
class MailBoxInvoke {
public:
  void invoke(int numQPUs, Code const &code, IntList const &params);
  void run();
  void schedule(Code const &code, IntList const &params);
};


class ScheduledJobs;

namespace vc4_invoke {

void run(ScheduledJobs &jobs);

} // vc4_invoke

}  // namespace V3DLib

#endif  // _V3DLIB_VC4_INVOKE_H_
