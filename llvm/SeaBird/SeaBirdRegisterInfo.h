#ifndef LLVM_LIB_TARGET_SEABIRD_SEABIRDREGISTERINFO_H
#define LLVM_LIB_TARGET_SEABIRD_SEABIRDREGISTERINFO_H

#include "MCTargetDesc/SeaBirdMCTargetDesc.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "SeaBirdGenRegisterInfo.inc"

namespace llvm {

class SeaBirdRegisterInfo final : public SeaBirdGenRegisterInfo {
public:
  SeaBirdRegisterInfo();

  const std::uint16_t *
  getCalleeSavedRegs(const MachineFunction *MF = nullptr) const override;
  const std::uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                            CallingConv::ID CC) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
};

} // namespace llvm

#endif
