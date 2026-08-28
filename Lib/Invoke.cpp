#include "Invoke.h"
#include "BaseKernel.h"
#include "CodeStruct.h"
#include "vc4/Invoke.h"

namespace V3DLib {

/**
 * Number of 32-bit words needed for the parameters (uniforms)
 *
 * - First two values are always the QPU ID and num QPU's
 * - Next come the actual kernel parameters, as defined in the user code
 * - This is terminated by a dummy uniform value, see Note 1.
 */
int ScheduledJob::num_params() const {
  return (2 + params.size() + 1);
}

namespace {

ScheduledJobs s_jobs;

}  // anon namespace


/**
 * @brief Determine the total number of parameters for all scheduled jobs.
 */
int ScheduledJobs::num_params() const {
  int ret = 0;

  for (auto const &job: *this) {
    ret += job.num_params();
  }

  assert(ret > 0);
  return ret;
}


/**
 * @brief Support for running kernels in parallel.
 *
 * Kernels are stored in a scheduling list and invoked together.
 *
 * `vc4` only; there is no direct option on `v3d` to run multiple kernels simultaneously.
 */
namespace Invoke {

/**
 * @brief Add passed kernel to list of kernels to run
 */
void schedule(BaseKernel const &k) {
  auto const &code   = k.compile().code_struct().code();
  auto const &params = k.params();

  assertq(!code.empty(), "schedule(): no code passed");
  auto item = ScheduledJob(code, params);
  s_jobs.push_back(item);
}


/**
 * @brief Run all kernels in the scheduling list synchronously.
 */
void run() {
  assertq(!s_jobs.empty(), "No scheduled jobs to run");
  assertq(Platform::max_qpus() >= (int) s_jobs.size(), "More scheduled jobs than QPU's present");

  if (Platform::compiling_for_vc4()) {
    vc4_invoke::run(s_jobs);
  } else {
    assertq(false, "Still need to implement scheduling for v3d");
  }

  s_jobs.clear();
}

}  // namespace Invoke
}  // namespace V3DLib
