#include "Functions.h"
#include "Source/Lang.h"
#include "Source/Stmt.h"
#include "Source/StmtStack.h"
#include "DMA/Operations.h"

/**
 * @file
 * Place to collect functions related to instruction generation on all levels.
 */

namespace V3DLib {
namespace vc4 {
namespace {

//=============================================================================
// Semaphore access
//=============================================================================

void semaInc(int semaId) {
  Stmt::Ptr s = Stmt::create(Stmt::SEMA_INC);
  s->dma.semaId(semaId);
  stmtStack() << s;
}

void semaDec(int semaId) {
  Stmt::Ptr s = Stmt::create(Stmt::SEMA_DEC);
  s->dma.semaId(semaId);
  stmtStack() << s;
}


//=============================================================================
// Host IRQ
//=============================================================================

void hostIRQ() {
  stmtStack() << Stmt::create(Stmt::SEND_IRQ_TO_HOST);
}


/**
 * @brief Wait for all QPU's to complete; `vc4`-specific.
 *
 * This is the original QPU wait routine, used in the final part of a kernel.
 * It is inadequate for usage as `barrier()`, see discussion below.
 *
 * The implementation uses semaphores internally.
 * This is code for the Source level. It should be called within kernels.
 *
 * ---------------------------------
 *
 * NOT WORKING AS BARRIER
 * ----------------------
 *
 * This is not a clean implementation of `barrier()`.
 *
 * Consider when QPU 1 comes before QPU 0: it zooms straight through.
 * Then, QPU 0 gets stuck forever on `SDEC 15`.
 *
 * If `QPU 0` comes first anyway, what can happen is that the next QPU increments
 * the semaphore while `QPU 0` is in the For-loop and passes.  
 * The best you can say is that `QPU 0` waits for all other QPU's to pass.
 *
 * 20260402: This is _exactly_ what happens for `vc4 Mandelbrot` with `1 < numQPUs < 4`.
 *           QPU 0 completes after one of the other QPU's.
 *
 *
 * Notes
 * -----
 *
 * Note that most semaphore stuff is under DMA.
 * Strictly speaking, this is not necessary, because semaphore are not explicitly linked to DMA.
 * But I see no reason to make semaphores more explicit.
 */
void wait_qpu() {
   // The issue here is that QPU 0 may not be the first QPU to reach this code.
  If (me() == 0)               
    Int n = numQPUs() - 1;       comment("Start vc4 barrier");
                                  comment("QPU 0 wait for other QPUs to finish");
    For (Int i = 0, i < n, i++)
      semaDec(15);
    End

    hostIRQ();                   comment("Send host IRQ");
  Else
    semaInc(15);
  End
}

} // anon namespace


/**
 * Add the postfix code to the vc kernel.
 *
 * This is code for the Source level. It should be called within kernels.
 */
void kernelFinish() {
  dmaWaitRead();                header("Kernel termination");
                                comment("Ensure outstanding DMAs have completed");
  dmaWaitWrite();
  wait_qpu();
}


}  // namespace vc4
}  // namespace V3DLib
