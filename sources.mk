#
# This file is generated!  Editing it directly is a bad idea.
#
# Generated on: Sun Nov 23 05:10:12 AM CET 2025
#
###############################################################################

# Library Object files - only used for LIB
OBJ := \
  Common/BufferObject.o  \
  Common/SharedArray.o  \
  Common/CompileData.o  \
  global/log.o  \
  Support/RegIdSet.o  \
  Support/BaseSettings.o  \
  Support/Timer.o  \
  Support/Settings.o  \
  Support/debug.o  \
  Support/pgm.o  \
  Support/Platform.o  \
  Support/basics.o  \
  Support/Helpers.o  \
  Support/HeapManager.o  \
  Support/InstructionComment.o  \
  KernelDriver.o  \
  Kernels/DotVector.o  \
  Kernels/ComplexDotVector.o  \
  Kernels/Rot3D.o  \
  Kernels/Matrix.o  \
  Kernels/Cursor.o  \
  Source/gather.o  \
  Source/Cond.o  \
  Source/Ptr.o  \
  Source/Complex.o  \
  Source/Stmt.o  \
  Source/Op.o  \
  Source/Functions.o  \
  Source/Pretty.o  \
  Source/Interpreter.o  \
  Source/StmtStack.o  \
  Source/Var.o  \
  Source/Lang.o  \
  Source/Expr.o  \
  Source/BExpr.o  \
  Source/Translate.o  \
  Source/OpItems.o  \
  Source/Int.o  \
  Source/Float.o  \
  Source/CExpr.o  \
  SourceTranslate.o  \
  Target/instr/Reg.o  \
  Target/instr/Imm.o  \
  Target/instr/Label.o  \
  Target/instr/Instr.o  \
  Target/instr/ALUOp.o  \
  Target/instr/ALUInstruction.o  \
  Target/instr/RegOrImm.o  \
  Target/instr/Conditions.o  \
  Target/instr/Mnemonics.o  \
  Target/EmuSupport.o  \
  Target/Pretty.o  \
  Target/BufferObject.o  \
  Target/Satisfy.o  \
  Target/Subst.o  \
  Target/SmallLiteral.o  \
  Target/Emulator.o  \
  vc4/DMA/DMA.o  \
  vc4/DMA/LoadStore.o  \
  vc4/DMA/Helpers.o  \
  vc4/DMA/Operations.o  \
  vc4/vc4.o  \
  vc4/RegAlloc.o  \
  vc4/KernelDriver.o  \
  vc4/BufferObject.o  \
  vc4/SourceTranslate.o  \
  vc4/Mailbox.o  \
  vc4/Encode.o  \
  vc4/PerformanceCounters.o  \
  vc4/RegisterMap.o  \
  vc4/Invoke.o  \
  Liveness/Range.o  \
  Liveness/Optimizations.o  \
  Liveness/CFG.o  \
  Liveness/Liveness.o  \
  Liveness/LiveSet.o  \
  Liveness/RegUsage.o  \
  Liveness/UseDef.o  \
  BaseKernel.o  \
  v3d/instr/RFAddress.o  \
  v3d/instr/Register.o  \
  v3d/instr/Instr.o  \
  v3d/instr/Location.o  \
  v3d/instr/SmallImm.o  \
  v3d/instr/Snippets.o  \
  v3d/instr/BaseSource.o  \
  v3d/instr/Encode.o  \
  v3d/instr/Source.o  \
  v3d/instr/OpItems.o  \
  v3d/instr/Mnemonics.o  \
  v3d/Driver.o  \
  v3d/KernelDriver.o  \
  v3d/BufferObject.o  \
  v3d/SourceTranslate.o  \
  v3d/v3d.o  \
  v3d/PerformanceCounters.o  \
  v3d/driver/screen.o  \
  v3d/driver/BOList.o  \
  v3d/driver/device_info.o  \
  v3d/RegisterMapping.o  \
  LibSettings.o  \
  vc4/dump_instr.o  \
  v3d/instr/v3d_api.o  \

# All programs in the Examples *and Tools* directory
EXAMPLES := \
  HeatMap  \
  GCD  \
  Tri  \
  ID  \
  DMA  \
  Rot3D  \
  Matrix  \
  Hello  \
  ReqRecv  \
  Counter  \
  OET  \
  Instruction  \
  Mandelbrot  \
  detectPlatform  \

# support files for tests
TESTS_FILES := \
  Tests/testRegMap.o  \
  Tests/testCmdLine.o  \
  Tests/testRot3D.o  \
  Tests/testPrefetch.o  \
  Tests/testDFT.o  \
  Tests/testMain.o  \
  Tests/support/rotate_kernel.o  \
  Tests/support/dft_support.o  \
  Tests/support/summation_kernel.o  \
  Tests/support/matrix_support.o  \
  Tests/support/disasm_kernel.o  \
  Tests/support/ProfileOutput.o  \
  Tests/support/support.o  \
  Tests/testV3d.o  \
  Tests/testDSL.o  \
  Tests/testFunctions.o  \
  Tests/testMatrix.o  \
  Tests/testBO.o  \
  Tests/testFFT.o  \
  Tests/testSFU.o  \
  Tests/testConditionCodes.o  \
  Tests/testImmediates.o  \
  Tests/support/qpu_disasm.o  \

