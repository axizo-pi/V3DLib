#
# This file is generated!  Editing it directly is a bad idea.
#
# Generated on: Mon May  4 06:25:36 PM CEST 2026
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
  Source/Lang.o  \
  Source/Translate.o  \
  Source/Functions.o  \
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
  v3d/UniformConstants.o  \
  vc4/Invoke.o  \
  vc4/vc4.o  \
  vc4/Mailbox.o  \
  vc4/Encode.o  \
  vc4/SourceTranslate.o  \
  vc4/Functions.o  \
  vc4/DMA/DMA.o  \
  vc4/DMA/VPMRequest.o  \
  vc4/DMA/VPMArray.o  \
  vc4/DMA/LoadStore.o  \
  vc4/DMA/Helpers.o  \
  vc4/DMA/Operations.o  \
  vc4/Instr.o  \
  vc4/RegisterMap.o  \
  vc4/PerformanceCounters.o  \
  vc4/KernelDriver.o  \
  vc4/RegAlloc.o  \
  vc4/BufferObject.o  \
  Emulator/QPUState.o  \
  Emulator/DMA.o  \
  Emulator/Interpreter.o  \
  Emulator/Debugger.o  \
  Emulator/Emulator.o  \
  Emulator/EmuSupport.o  \
  Emulator/Mutex.o  \
  Emulator/DMAAddr.o  \
  Emulator/EmuState.o  \
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
  Support/bmp.o  \
  Support/Platform.o  \
  v3d/instr/v3d_api.o  \
  vc4/dump_instr.o  \

# All programs in the Examples *and Tools* directory
EXAMPLES := \
  Concurrent  \
  DMA  \
  GCD  \
  Hello  \
  ID  \
  Matrix  \
  OET  \
  ReqRecv  \
  Tri  \
  Counter  \
  detectPlatform  \
  Instruction  \
  KernelParams  \
  Sub  \

# support files for tests
TESTS_FILES := \
  Tests/testBO.o  \
  Tests/testVPM.o  \
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
  Tests/testMutex.o  \
  Tests/testDFT.o  \
  Tests/testFFT.o  \
  Tests/testImmediates.o  \
  Tests/testFunctions.o  \
  Tests/testSFU.o  \
  Tests/testMatrix.o  \
  Tests/testConditionCodes.o  \
  Tests/testMain.o  \
  Tests/testLog.o  \
  Tests/testLoop.o  \
  Tests/support/qpu_disasm.o  \

#
# sub-projects
#
SUB_PROJECTS := \
  HeatMap \
  Gravity \
  LSTM \
  Mandelbrot \
  Lib \
  RNN \
  GRU \
  Rot3D \


HeatMap: $(V3DLIB)
	@cd Examples/HeatMap && make DEBUG=${DEBUG}
	
Gravity: $(V3DLIB)
	@cd Examples/Gravity && make DEBUG=${DEBUG}
	
LSTM: $(V3DLIB)
	@cd Examples/LSTM && make DEBUG=${DEBUG}
	
Mandelbrot: $(V3DLIB)
	@cd Examples/Mandelbrot && make DEBUG=${DEBUG}
	
Lib: $(V3DLIB)
	@cd Examples/RNN/Lib && make DEBUG=${DEBUG}
	
RNN: $(V3DLIB)
	@cd Examples/RNN && make DEBUG=${DEBUG}
	
GRU: $(V3DLIB)
	@cd Examples/NN/GRU && make DEBUG=${DEBUG}
	
Rot3D: $(V3DLIB)
	@cd Examples/Rot3D && make DEBUG=${DEBUG}
	

