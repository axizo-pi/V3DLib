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

} // anon namespace


/**
 * `vc4` implementation of `barrier()`.
 *
 * The implementation uses semaphores internally.
 * This is code for the Source level. It should be called within kernels.
 *
 *
 * @param do_irq  if set, set a host interrupt
 *
 * ---------------------------------
 *
 * NOT WORKING
 * -----------

 * The final `barrier()` call on end of program is OK, within the 
 * program it blocks (See Gravity example).
 *
 * The error on `vc4`:
 *
 *     ERROR: mbox_property() ioctl failed, ret: -1, error: Unknown error -1
 *     ERROR: execute_qpu(): mbox_property call failed
 *
 * Making the host interrupt conditional didn't help.
 * Obviously, I don't understand it well enough.
 *
 * NOTES
 * -----
 *
 * Note that most semaphore stuff is under DMA.
 * Strictly speaking, this is not necessary, because semaphore are not explicitly linked to DMA.
 * But I see no reason to make semaphores more explicit.
 */
void barrier(bool do_irq) {
  If (me() == 0)               
    Int n = numQPUs() - 1;       comment("Start vc4 barrier");
		 	                           comment("QPU 0 wait for other QPUs to finish");
    For (Int i = 0, i < n, i++)
      semaDec(15);
    End

    if (do_irq) {
      hostIRQ();                   comment("Send host IRQ");
    }
  Else
    semaInc(15);
  End
}


/**
 * Add the postfix code to the vc kernel.
 *
 * This is code for the Source level. It should be called within kernels.
 */
void kernelFinish() {
  dmaWaitRead();                header("Kernel termination");
                                comment("Ensure outstanding DMAs have completed");
  dmaWaitWrite();
	barrier(true);
}


}  // namespace vc4
}  // namespace V3DLib
