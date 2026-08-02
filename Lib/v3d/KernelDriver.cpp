#include "KernelDriver.h"
#include "UniformConstants.h"

namespace V3DLib {
namespace v3d {

// WEIRDNESS, due to changes, this file did not compile because it suddenly couldn't find
// the relevant overload of operator <<.
// Adding this solved it. HUH??
// Also: the << definitions in `basics.h` DID get picked up; the std::string versions did not.
using ::operator<<; // C++ weirdness

namespace {

void load_uniforms(
  Data &unif,
  int numQPUs,
  Data const &devnull,
  Data const &done,
  IntList const &params
) {
  UniformConstants uc =  uniform_constants.list();
  if (!uc.empty()) {
    warn << "load_uniforms() " << uc.size() << " uniform constants";
  }

  unif.alloc(params.size() + 4 + (int) uc.size());

  int offset = 0;

  // Add the common uniforms
  unif[offset++] = 0;                     // qpu number (id for current qpu) - 0 is for 1 QPU
  unif[offset++] = numQPUs;               // num qpu's running for this job
  unif[offset++] = devnull.getAddress();  // Memory location for values to be discarded

  for (int j = 0; j < params.size(); j++) {
   unif[offset++] = params[j];
 }

/*
  {
    std::string buf;
    buf << "load_uniforms:";
    for (int j = 0; j < params.size(); j++) {
      buf << "\n  param " << j << ": " << params[j];
    }
    warn << buf;
  }
*/  

  uc.load(unif, offset);

  // The last item is for the 'done' location;
  unif[offset] = (uint32_t) done.getAddress();

/*  
  {
    std::string buf = "load_uniforms post:";
    for (int j = 0; j <= offset; j++) {
      buf << "\n  " << j << ": " << unif[j]
          << " (" << *((float *) &unif[j]) << ")" ;
    }
    warn << buf;
  }
*/  
}


} // anon namespace


///////////////////////////////////////////////////////////////////////////////
// Class KernelDriver
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Invoke kernel on QPUs
 */
void KernelDriver::invoke(V3DLib::Compile &code, int numQPUs, IntList &params, bool wait_complete) {
  assert(params.size() != 0);

  if (code.has_errors()) {
    fatal("Errors during kernel compilation/encoding, can't continue.");
  }

  if (numQPUs <= 0) {
      cerr << "Zero or negative QPU's selected" << thrw;
  }

  if (Platform::compiling_for_vc7()) {
    if (numQPUs > 16) {
      cerr << "Num QPU's exceeded; Max QPU's is 16 for vc7" << thrw;
    }
  } else {
    if (numQPUs != 1 && numQPUs != 8) {
      cerr << "Num QPU's must be 1 or 8 for vc6" << thrw;
    }
  }

  assertq(!code.has_errors(), "v3d kernels has errors, can not invoke");

  code.allocate();
  //assert(m_code.allocated());
  assert(!code.code().empty());

  if (!devnull.allocated()) {
    devnull.alloc(16);
  }

  done.alloc(1);
  done[0] = 0;

  load_uniforms(uniforms, numQPUs, devnull, done, params);

  drv.add_bo(getBufferObject().getHandle());
  drv.execute(code.code(), &uniforms, numQPUs, wait_complete);
}


void KernelDriver::wait_complete() {
  if (drv.num_handles() == 0) {
    warn << "wait_complete(): nothing to wait for";
    return;
  }

  warn << "wait_complete done: " << done[0];
  warn << "wait_complete(): waiting for completion.";

  drv.wait_bo();
}

}  // namespace v3d
}  // namespace V3DLib
