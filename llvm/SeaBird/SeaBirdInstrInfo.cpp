#include "SeaBirdInstrInfo.h"
#include "SeaBirdSubtarget.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_INSTRINFO_CTOR_DTOR
#include "SeaBirdGenInstrInfo.inc"

using namespace llvm;

SeaBirdInstrInfo::SeaBirdInstrInfo(const SeaBirdSubtarget &STI)
    : SeaBirdGenInstrInfo(STI, RegisterInfo), RegisterInfo() {}

void SeaBirdInstrInfo::copyPhysReg(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    const DebugLoc &DL, Register Destination, Register Source, bool KillSource,
    bool RenamableDest, bool RenamableSrc) const {
  const bool GPR = SeaBird::GPR64RegClass.contains(Destination) &&
                   SeaBird::GPR64RegClass.contains(Source);
  const bool Vector = SeaBird::VR128RegClass.contains(Destination) &&
                      SeaBird::VR128RegClass.contains(Source);
  if (!GPR && !Vector)
    llvm_unreachable("unsupported SeaBird physical-register copy");
  const bool Is64Bit =
      MBB.getParent()->getSubtarget<SeaBirdSubtarget>().is64Bit();
  const unsigned CopyOpcode =
      !GPR ? SeaBird::MOVV128
           : (Is64Bit ? SeaBird::MOVrr : SeaBird::MOVrr32);
  BuildMI(MBB, Position, DL, get(CopyOpcode), Destination)
      .addReg(Source, getKillRegState(KillSource));
}

void SeaBirdInstrInfo::storeRegToStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    Register Source, bool IsKill, int FrameIndex,
    const TargetRegisterClass *RC, Register VReg,
    MachineInstr::MIFlag Flags) const {
  const DebugLoc DL =
      Position == MBB.end() ? DebugLoc() : Position->getDebugLoc();
  const bool Is64Bit =
      MBB.getParent()->getSubtarget<SeaBirdSubtarget>().is64Bit();
  unsigned Opcode;
  if (RC == &SeaBird::FPR32RegClass)
    Opcode = SeaBird::FST32CG;
  else if (RC == &SeaBird::FPR64RegClass)
    Opcode = SeaBird::FST64;
  else if (RC == &SeaBird::FPR128RegClass)
    Opcode = SeaBird::FST128CG;
  else if (SeaBird::VR128RegClass.hasSubClassEq(RC))
    Opcode = SeaBird::VST128;
  else if (SeaBird::GPR64RegClass.hasSubClassEq(RC))
    Opcode = Is64Bit ? SeaBird::STQrr : SeaBird::STWrr32;
  else
    llvm_unreachable("unsupported SeaBird spill register class");
  BuildMI(MBB, Position, DL, get(Opcode))
      .addFrameIndex(FrameIndex)
      .addReg(SeaBird::NOIDX)
      .addImm(1)
      .addImm(0)
      .addReg(Source, getKillRegState(IsKill))
      .setMIFlag(Flags);
}

void SeaBirdInstrInfo::loadRegFromStackSlot(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator Position,
    Register Destination, int FrameIndex, const TargetRegisterClass *RC,
    Register VReg, unsigned SubReg, MachineInstr::MIFlag Flags) const {
  const DebugLoc DL =
      Position == MBB.end() ? DebugLoc() : Position->getDebugLoc();
  const bool Is64Bit =
      MBB.getParent()->getSubtarget<SeaBirdSubtarget>().is64Bit();
  unsigned Opcode;
  if (RC == &SeaBird::FPR32RegClass)
    Opcode = SeaBird::FLD32CG;
  else if (RC == &SeaBird::FPR64RegClass)
    Opcode = SeaBird::FLD64;
  else if (RC == &SeaBird::FPR128RegClass)
    Opcode = SeaBird::FLD128CG;
  else if (SeaBird::VR128RegClass.hasSubClassEq(RC))
    Opcode = SeaBird::VLD128;
  else if (SeaBird::GPR64RegClass.hasSubClassEq(RC))
    Opcode = Is64Bit ? SeaBird::LDQrr : SeaBird::LDWrr32;
  else
    llvm_unreachable("unsupported SeaBird reload register class");
  BuildMI(MBB, Position, DL, get(Opcode), Destination)
      .addFrameIndex(FrameIndex)
      .addReg(SeaBird::NOIDX)
      .addImm(1)
      .addImm(0)
      .setMIFlag(Flags);
}
