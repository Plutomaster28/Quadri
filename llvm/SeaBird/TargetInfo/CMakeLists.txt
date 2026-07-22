add_llvm_component_library(LLVMSeaBirdInfo
  SeaBirdTargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  SeaBird
  )
