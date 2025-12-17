#include "Invoke.h"
#include "../Invoke.h"
#include "Mailbox.h"
#include "vc4.h"
#include "defines.h"
#include "LibSettings.h"
#include "Support/Platform.h"

using namespace Log;

namespace V3DLib {

namespace {

void check_align(BaseSharedArray const &arr, std::string const &name) {
  warn << "check_align called for '" << name << "'";

  if (arr.getAddress() % 16) {
    cerr << "SharedArray " << name << " not 16-bit aligned";
  }
}


void check_align(uint32_t addr, std::string const &name) {
  warn << "check_align addr called for '" << name << "'";

  if (addr % 16) {
    cerr << "Address " << name << " not 16-bit aligned: 0x" << hex << addr;
  }
}


/**
 * Number of 32-bit words needed for the parameters (uniforms)
 *
 * - First two values are always the QPU ID and num QPU's
 * - Next come the actual kernel parameters, as defined in the user code
 * - This is terminated by a dummy uniform value, see Note 1.
 */
int num_params(IntList const &params) {
  assert(!params.empty());
  return (2 + params.size() + 1);
}


/**
 * Initialize uniforms to pass into running QPUs for vc4
 *
 * The number and types of parameters will not change for a given kernel.
 * The value of the parameters, however, can change, so this needs to be reset every time.
 *
 * All uniform values are the same for all QPUs, *except* the qpu id.
 *
 * ----------------------------------------------------------------------------
 * Notes
 * =====
 *
 * 1. Often (not always!), the final param is passed garbled when input as 
 *    a uniform to a kernel program executing on vc4 hardware.
 *    It appears to happen to direct Float/Int values.
 *    After spending days on this with paranoid debugging, I could not find the
 *    cause and gave up. Instead, I'll just pass a final dummy uniform value,
 *    which can be mangled to the heart's content of the hardware.
 */
void load_uniforms(Data &uniforms, IntList const &params, int numQPUs) {
  assert(0 < numQPUs && numQPUs <= Platform::max_qpus());

  if (!uniforms.allocated()) {
    uniforms.alloc(num_params(params)*Platform::max_qpus());
  } else {
    assert((int) uniforms.size() == num_params(params)*Platform::max_qpus());
  }

  int offset = 0;
  for (int i = 0; i < numQPUs; i++) {
    uniforms[offset++] = (uint32_t) i;              // Unique QPU ID
    uniforms[offset++] = (uint32_t) numQPUs;        // QPU count

    for (int j = 0; j < params.size(); j++) {
      uniforms[offset++] = params[j];
    }

    uniforms[offset++] = 0;                         // Dummy final parameter, see Note 1.
  }

  assert(offset == num_params(params)*numQPUs);
}


void load_uniforms(int &offset, int index, int qpu_count,  Data &uniforms, ScheduledJob &job) {
  assert(uniforms.allocated());

	job.params_offset = offset;
	//warn << "load_uniforms jobs.params_offset: " << job.params_offset;

  uniforms[offset++] = (uint32_t) index;            // Unique QPU ID
  uniforms[offset++] = (uint32_t) qpu_count;        // QPU count

  for (int j = 0; j < job.params.size(); j++) {
    uniforms[offset++] = job.params[j];
  }

  uniforms[offset++] = 0;                           // Dummy final parameter, see Note 1.
}


void alloc_launch_messages(Data &launch_messages) {
  if (!launch_messages.allocated()) {
  	launch_messages.alloc(2*Platform::max_qpus());  // Go for the max size, irrespective of number of jobs
	}
}


void add_launch_message(int index, Data &launch_messages, ScheduledJob const &job, Data const &uniforms) {
  assertq(!uniforms.empty(), "add_launch_messages(): expecting values for uniforms");
	assert(job.params_offset != -1);

	alloc_launch_messages(launch_messages);
  check_align(launch_messages, "launch_messages");

	launch_messages[2*index]     = uniforms.getAddress() + 4*job.params_offset;
	launch_messages[2*index + 1] = job.code.getAddress();
}


/**
 * Initialize launch messages, if not already done so
 *
 * Doing this for max number of QPUs, so that num QPUs can be changed dynamically on calls.
 */
void init_launch_messages(Data &launch_messages, Code const &code, IntList const &params, Data const &uniforms) {
  assertq(!uniforms.empty(), "init_launch_messages(): expecting values for uniforms");
  warn << "init_launch_messages() called";

	alloc_launch_messages(launch_messages);

  for (int i = 0; i < Platform::max_qpus(); i++) {
    uint32_t offset= uniforms.getAddress() + 4*i*num_params(params);  // 4* for uint32_t offset
    check_align(offset, "launch param");

    launch_messages[2*i]     = offset;
    launch_messages[2*i + 1] = code.getAddress();
  }
}


void check_platform() {
#ifdef QPU_MODE
  if (!Platform::is_pi_platform()) {
  	cerr << "check_platform(): will not run on this platform, only on Raspberry Pi's.";
	  cerr << "Can not run kernel on QPUs." << thrw;
	}
#else 
 	cerr << "check_platform(): will not run, QPU mode not enabled.";
  cerr << "Can not run kernel on QPUs." << thrw;
#endif
}


/**
 * Run the kernel on vc4 hardware
 */
void invoke_jobs(int numQPUs, Data const &launch_messages) {
	check_platform();

  enableQPUs();

  unsigned result = execute_qpu(
    getMailbox(),
    numQPUs,
    launch_messages.getAddress(),
    1,
    LibSettings::qpu_timeout()*1000
  );

  disableQPUs();

  if (result != 0) {
    cerr << "invoke_jobs(): Failed to invoke kernel on QPUs";
  }
}


/**
 * Container for launch info per QPU to run
 *
 * Array consecutively containing two values per QPU to run:
 *  - pointer to uniform parameters to pass per QPU
 *  - Start of code block to run per QPU
 */
Data launch_messages;

}  // anon namespace


/**
 * Load and run a single kernel on multiple QPU's.
 */
void MailBoxInvoke::invoke(int numQPUs, Code const &code, IntList const &params) {
  assertq(!code.empty(), "MailBoxInvoke::invoke(): no code to invoke", true );

  Data uniforms;  // Memory region for QPU parameters
  load_uniforms(uniforms, params, numQPUs);

  check_align(uniforms, "uniforms");
  check_align(launch_messages, "launch_messages");
  check_align(code, "code");

  init_launch_messages(launch_messages, code, params, uniforms);

  invoke_jobs(numQPUs, launch_messages);
}


namespace vc4_invoke {

/**
 * Load and run the scheduled jobs, each on one QPU
 */
void run(ScheduledJobs &jobs) {
  Data uniforms;                          // Memory region for QPU parameters
  uniforms.alloc(jobs.num_params());

  check_align(uniforms, "uniforms");

	int index = 0;
	int offset = 0;
	for (auto &job: jobs) {
  	load_uniforms(offset, index, (int) jobs.size(), uniforms, job);
		index++;
	}

	index = 0;
	for (auto &job: jobs) {
		add_launch_message(index, launch_messages, job, uniforms);
		index++;
	}

	invoke_jobs((int) jobs.size(), launch_messages);
}

} // namespace vc4_invoke
} // namespace V3DLib
