include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_llvm_component_library(LLVMSeaBirdDisassembler
  SeaBirdDisassembler.cpp

  LINK_COMPONENTS
  SeaBirdDesc
  SeaBirdInfo
  MC
  MCDisassembler
  Support

  ADD_TO_COMPONENT
  SeaBird
  )
