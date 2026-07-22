#ifndef LLVM_LIB_TARGET_SEABIRD_SEABIRDINSTRINFO_H
#define LLVM_LIB_TARGET_SEABIRD_SEABIRDINSTRINFO_H

#include "SeaBirdRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "SeaBirdGenInstrInfo.inc"

namespace llvm {

class SeaBirdSubtarget;

class SeaBirdInstrInfo final : public SeaBirdGenInstrInfo {
  SeaBirdRegisterInfo RegisterInfo;

public:
  explicit SeaBirdInstrInfo(const SeaBirdSubtarget &STI);

  const SeaBirdRegisterInfo &getRegisterInfo() const { return RegisterInfo; }
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
                   const DebugLoc &DL, Register Destination,
                   Register Source, bool KillSource, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;
  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
      Register Source, bool IsKill, int FrameIndex,
      const TargetRegisterClass *RC, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
      Register Destination, int FrameIndex, const TargetRegisterClass *RC,
      Register VReg, unsigned SubReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
};

} // namespace llvm

#endif
