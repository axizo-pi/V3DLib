#
# This file is generated!  Editing it directly is a bad idea.
#
# Generated on: Tue Aug 18 07:29:16 AM CEST 2026
#
###############################################################################

# Library Object files - only used for LIB
OBJ := \
  Compile.o  \
  global/log.o  \
  SourceTranslate.o  \
  vc4/BufferObject.o  \
  vc4/Compile.o  \
  vc4/RegAlloc.o  \
  vc4/KernelDriver.o  \
  vc4/DMA/Helpers.o  \
  vc4/DMA/Operations.o  \
  vc4/DMA/VPMArray.o  \
  vc4/DMA/DMA.o  \
  vc4/DMA/VPMRequest.o  \
  vc4/DMA/LoadStore.o  \
  vc4/SourceTranslate.o  \
  vc4/Mailbox.o  \
  vc4/PerformanceCounters.o  \
  vc4/vc4.o  \
  vc4/Encode.o  \
  vc4/Instr.o  \
  vc4/RegisterMap.o  \
  vc4/Functions.o  \
  vc4/Invoke.o  \
  Emulator/DMAAddr.o  \
  Emulator/Debugger.o  \
  Emulator/Interpreter.o  \
  Emulator/EmuState.o  \
  Emulator/Mutex.o  \
  Emulator/EmuSupport.o  \
  Emulator/DMA.o  \
  Emulator/QPUState.o  \
  Emulator/Emulator.o  \
  Kernels/Cursor.o  \
  Kernels/DotVector.o  \
  Kernels/Rot3D.o  \
  Kernels/Matrix.o  \
  Kernels/ComplexDotVector.o  \
  v3d/BufferObject.o  \
  v3d/Compile.o  \
  v3d/KernelDriver.o  \
  v3d/instr/Snippets.o  \
  v3d/instr/RFAddress.o  \
  v3d/instr/Mnemonics.o  \
  v3d/instr/SmallImm.o  \
  v3d/instr/BaseSource.o  \
  v3d/instr/Location.o  \
  v3d/instr/OpItems.o  \
  v3d/instr/Register.o  \
  v3d/instr/Encode.o  \
  v3d/instr/Instr.o  \
  v3d/instr/Source.o  \
  v3d/SourceTranslate.o  \
  v3d/v3d.o  \
  v3d/driver/screen.o  \
  v3d/driver/BOList.o  \
  v3d/driver/device_info.o  \
  v3d/RegisterMapping.o  \
  v3d/PerformanceCounters.o  \
  v3d/UniformConstants.o  \
  v3d/Combine.o  \
  v3d/Driver.o  \
  LibSettings.o  \
  BaseKernel.o  \
  Source/Float.o  \
  Source/StmtStack.o  \
  Source/Var.o  \
  Source/Cond.o  \
  Source/Lang.o  \
  Source/Complex.o  \
  Source/Expr.o  \
  Source/CExpr.o  \
  Source/gather.o  \
  Source/OpItems.o  \
  Source/Stmt.o  \
  Source/Int.o  \
  Source/Op.o  \
  Source/BExpr.o  \
  Source/Translate.o  \
  Source/Functions.o  \
  Source/Ptr.o  \
  Liveness/RegUsage.o  \
  Liveness/LiveSet.o  \
  Liveness/UseDef.o  \
  Liveness/Range.o  \
  Liveness/CFG.o  \
  Liveness/Optimizations.o  \
  Liveness/Liveness.o  \
  Target/BufferObject.o  \
  Target/instr/ALUInstruction.o  \
  Target/instr/Reg.o  \
  Target/instr/Mnemonics.o  \
  Target/instr/ALUOp.o  \
  Target/instr/RegOrImm.o  \
  Target/instr/Label.o  \
  Target/instr/Imm.o  \
  Target/instr/Instr.o  \
  Target/instr/Conditions.o  \
  Target/SmallLiteral.o  \
  Target/Subst.o  \
  Target/Satisfy.o  \
  Support/Helpers.o  \
  Support/basics.o  \
  Support/BaseSettings.o  \
  Support/Platform.o  \
  Support/InstructionComment.o  \
  Support/debug.o  \
  Support/RegIdSet.o  \
  Support/HeapManager.o  \
  Support/Settings.o  \
  Support/pgm.o  \
  Support/bmp.o  \
  Support/Timer.o  \
  Common/BufferObject.o  \
  Common/SharedArray.o  \
  Common/CompileData.o  \
  Invoke.o  \
  vc4/dump_instr.o  \
  v3d/instr/v3d_api.o  \

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
  Tests/testConversions.o  \
  Tests/testRegMap.o  \
  Tests/support/support.o  \
  Tests/support/dft_support.o  \
  Tests/support/summation_kernel.o  \
  Tests/support/ProfileOutput.o  \
  Tests/support/disasm_kernel.o  \
  Tests/support/matrix_support.o  \
  Tests/support/rotate_kernel.o  \
  Tests/testBO.o  \
  Tests/testFFT.o  \
  Tests/testImmediates.o  \
  Tests/testMutex.o  \
  Tests/testMatrix.o  \
  Tests/testConditionCodes.o  \
  Tests/testDFT.o  \
  Tests/testLoop.o  \
  Tests/testV3d.o  \
  Tests/testCLZ.o  \
  Tests/testDSL.o  \
  Tests/testMain.o  \
  Tests/testRot3D.o  \
  Tests/testFunctions.o  \
  Tests/testSFU.o  \
  Tests/testTrig.o  \
  Tests/testCmdLine.o  \
  Tests/testPrefetch.o  \
  Tests/testVPM.o  \
  Tests/testLog.o  \
  Tests/support/qpu_disasm.o  \

#
# sub-projects
#
SUB_PROJECTS := \
  Rot3D \
  Gravity \
  Lib \
  GRU \
  LSTM \
  RNN \
  HeatMap \
  Mandelbrot \
  RayTracing \


Rot3D: $(V3DLIB)
	@cd Examples/Rot3D && make DEBUG=${DEBUG}
	
Gravity: $(V3DLIB)
	@cd Examples/Gravity && make DEBUG=${DEBUG}
	
Lib: $(V3DLIB)
	@cd Examples/NN/Lib && make DEBUG=${DEBUG}
	
GRU: $(V3DLIB)
	@cd Examples/NN/GRU && make DEBUG=${DEBUG}
	
LSTM: $(V3DLIB)
	@cd Examples/NN/LSTM && make DEBUG=${DEBUG}
	
RNN: $(V3DLIB)
	@cd Examples/NN/RNN && make DEBUG=${DEBUG}
	
HeatMap: $(V3DLIB)
	@cd Examples/HeatMap && make DEBUG=${DEBUG}
	
Mandelbrot: $(V3DLIB)
	@cd Examples/Mandelbrot && make DEBUG=${DEBUG}
	
RayTracing: $(V3DLIB)
	@cd Examples/RayTracing && make DEBUG=${DEBUG}
	

