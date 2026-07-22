add_llvm_component_library(LLVMSeaBirdDesc
  SeaBirdAsmBackend.cpp
  SeaBirdELFObjectWriter.cpp
  SeaBirdInstPrinter.cpp
  SeaBirdMCAsmInfo.cpp
  SeaBirdMCCodeEmitter.cpp
  SeaBirdMCTargetDesc.cpp

  LINK_COMPONENTS
  SeaBirdInfo
  MC
  MCDisassembler
  Support
  TargetParser

  ADD_TO_COMPONENT
  SeaBird
  )
