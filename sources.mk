#
# This file is generated!  Editing it directly is a bad idea.
#
# Generated on: Fri Dec 12 04:26:03 AM CET 2025
#
###############################################################################

# Library Object files - only used for LIB
OBJ := \
  Invoke.o  \
  Common/SharedArray.o  \
  Common/CompileData.o  \
  Common/BufferObject.o  \
  BaseKernel.o  \
  Liveness/RegUsage.o  \
  Liveness/Optimizations.o  \
  Liveness/Liveness.o  \
  Liveness/UseDef.o  \
  Liveness/CFG.o  \
  Liveness/LiveSet.o  \
  Liveness/Range.o  \
  SourceTranslate.o  \
  LibSettings.o  \
  KernelDriver.o  \
  Source/CExpr.o  \
  Source/Float.o  \
  Source/Int.o  \
  Source/Stmt.o  \
  Source/Expr.o  \
  Source/BExpr.o  \
  Source/Interpreter.o  \
  Source/Lang.o  \
  Source/Translate.o  \
  Source/Functions.o  \
  Source/Pretty.o  \
  Source/OpItems.o  \
  Source/Op.o  \
  Source/Cond.o  \
  Source/StmtStack.o  \
  Source/Var.o  \
  Source/Complex.o  \
  Source/gather.o  \
  Source/Ptr.o  \
  Kernels/Rot3D.o  \
  Kernels/Cursor.o  \
  Kernels/ComplexDotVector.o  \
  Kernels/Matrix.o  \
  Kernels/DotVector.o  \
  global/log.o  \
  Target/SmallLiteral.o  \
  Target/instr/ALUInstruction.o  \
  Target/instr/Imm.o  \
  Target/instr/RegOrImm.o  \
  Target/instr/ALUOp.o  \
  Target/instr/Reg.o  \
  Target/instr/Mnemonics.o  \
  Target/instr/Label.o  \
  Target/instr/Instr.o  \
  Target/instr/Conditions.o  \
  Target/Satisfy.o  \
  Target/Subst.o  \
  Target/Emulator.o  \
  Target/EmuSupport.o  \
  Target/BufferObject.o  \
  v3d/RegisterMapping.o  \
  v3d/instr/Encode.o  \
  v3d/instr/Mnemonics.o  \
  v3d/instr/Snippets.o  \
  v3d/instr/Register.o  \
  v3d/instr/Location.o  \
  v3d/instr/Source.o  \
  v3d/instr/RFAddress.o  \
  v3d/instr/OpItems.o  \
  v3d/instr/Instr.o  \
  v3d/instr/BaseSource.o  \
  v3d/instr/SmallImm.o  \
  v3d/v3d.o  \
  v3d/driver/device_info.o  \
  v3d/driver/BOList.o  \
  v3d/driver/screen.o  \
  v3d/SourceTranslate.o  \
  v3d/Combine.o  \
  v3d/Driver.o  \
  v3d/PerformanceCounters.o  \
  v3d/KernelDriver.o  \
  v3d/BufferObject.o  \
  vc4/Invoke.o  \
  vc4/vc4.o  \
  vc4/Mailbox.o  \
  vc4/Encode.o  \
  vc4/SourceTranslate.o  \
  vc4/DMA/DMA.o  \
  vc4/DMA/LoadStore.o  \
  vc4/DMA/Helpers.o  \
  vc4/DMA/Operations.o  \
  vc4/RegisterMap.o  \
  vc4/PerformanceCounters.o  \
  vc4/KernelDriver.o  \
  vc4/RegAlloc.o  \
  vc4/BufferObject.o  \
  Support/HeapManager.o  \
  Support/basics.o  \
  Support/debug.o  \
  Support/InstructionComment.o  \
  Support/RegIdSet.o  \
  Support/Settings.o  \
  Support/Timer.o  \
  Support/pgm.o  \
  Support/Helpers.o  \
  Support/BaseSettings.o  \
  Support/Platform.o  \
  v3d/instr/v3d_api.o  \
  vc4/dump_instr.o  \

# All programs in the Examples *and Tools* directory
EXAMPLES := \
  Concurrent  \
  Counter  \
  DMA  \
  GCD  \
  HeatMap  \
  Hello  \
  ID  \
  Instruction  \
  Matrix  \
  OET  \
  ReqRecv  \
  RNN  \
  Rot3D  \
  Tri  \
  detectPlatform  \

# support files for tests
TESTS_FILES := \
  Tests/testBO.o  \
  Tests/testPrefetch.o  \
  Tests/testCmdLine.o  \
  Tests/testDSL.o  \
  Tests/testV3d.o  \
  Tests/testRegMap.o  \
  Tests/testRot3D.o  \
  Tests/support/dft_support.o  \
  Tests/support/ProfileOutput.o  \
  Tests/support/support.o  \
  Tests/support/matrix_support.o  \
  Tests/support/summation_kernel.o  \
  Tests/support/rotate_kernel.o  \
  Tests/support/disasm_kernel.o  \
  Tests/testDFT.o  \
  Tests/testFFT.o  \
  Tests/testImmediates.o  \
  Tests/testFunctions.o  \
  Tests/testSFU.o  \
  Tests/testMatrix.o  \
  Tests/testConditionCodes.o  \
  Tests/testMain.o  \
  Tests/support/qpu_disasm.o  \

#
# sub-projects
#
SUB_PROJECTS := \
  Mandelbrot \



Mandelbrot:
	cd Examples/Mandelbrot && make DEBUG=${DEBUG} QPU=${QPU}

	

