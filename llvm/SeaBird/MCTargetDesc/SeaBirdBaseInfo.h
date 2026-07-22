#ifndef LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDBASEINFO_H
#define LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDBASEINFO_H

#include "SeaBirdMCTargetDesc.h"

namespace llvm::SeaBirdII {

enum Form : unsigned {
  FormMask = 0x1F00,
  FormShift = 8,
  CopyRR = 1,
  ALURR = 2,
  CompareRR = 3,
  RI64 = 4,
  BranchRel32 = 5,
  NoOperand = 6,
  LoadQ = 7,
  StoreQ = 8,
  IndirectCall = 9,
  FPBinary = 10,
  VBinary = 11,
  CopyVV = 12,
  ScalarFPLoad = 13,
  ScalarFPStore = 14,
  AddressCalc = 15,
  VectorLoad = 16,
  VectorStore = 17,
  IntToFP = 18,
  FPToInt = 19,
  UnaryInPlace = 20,
  UnaryRR = 21,
  ALURI = 22,
  CompareRI = 23,
  TrapImm = 24,
  RegCondBranch = 25,
  StackReg = 26,
  FrameImm = 27,
  TernaryRI = 28,
  TernaryByte = 29,
  ReadReg = 30,
  ControlReg = 31,
};

constexpr unsigned OpcodeMask = 0xFF;
constexpr unsigned ExtensionGroupMask = 0x1FE000;
constexpr unsigned ExtensionGroupShift = 13;
constexpr unsigned ScalarWidthMask = 0xE00000;
constexpr unsigned ScalarWidthShift = 21;

} // namespace llvm::SeaBirdII

#endif
