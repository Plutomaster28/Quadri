include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_llvm_component_library(LLVMSeaBirdAsmParser
  SeaBirdAsmParser.cpp

  LINK_COMPONENTS
  SeaBirdDesc
  SeaBirdInfo
  MC
  MCParser
  Support

  ADD_TO_COMPONENT
  SeaBird
  )

add_dependencies(LLVMSeaBirdAsmParser SeaBirdCommonTableGen)
