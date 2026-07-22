add_llvm_component_group(SeaBird)

set(LLVM_TARGET_DEFINITIONS SeaBird.td)

tablegen(LLVM SeaBirdGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM SeaBirdGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM SeaBirdGenCallingConv.inc -gen-callingconv)
tablegen(LLVM SeaBirdGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM SeaBirdGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM SeaBirdGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM SeaBirdGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(SeaBirdCommonTableGen)

add_llvm_target(SeaBirdCodeGen
  SeaBirdAsmPrinter.cpp
  SeaBirdFrameLowering.cpp
  SeaBirdISelDAGToDAG.cpp
  SeaBirdISelLowering.cpp
  SeaBirdInstrInfo.cpp
  SeaBirdMCInstLower.cpp
  SeaBirdMachineFunctionInfo.cpp
  SeaBirdRegisterInfo.cpp
  SeaBirdSubtarget.cpp
  SeaBirdTargetMachine.cpp

  LINK_COMPONENTS
  SeaBirdInfo
  Analysis
  AsmPrinter
  CodeGen
  CodeGenTypes
  Core
  MC
  SelectionDAG
  Support
  Target
  TargetParser

  ADD_TO_COMPONENT
  SeaBird
  )

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
