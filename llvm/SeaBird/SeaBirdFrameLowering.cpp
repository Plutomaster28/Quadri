#include "SeaBirdFrameLowering.h"
#include "SeaBirdInstrInfo.h"
#include "SeaBirdSubtarget.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/RegisterScavenging.h"

using namespace llvm;

bool SeaBirdFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  return MFI.hasVarSizedObjects() || MFI.isFrameAddressTaken();
}

void SeaBirdFrameLowering::emitPrologue(MachineFunction &MF,
                                         MachineBasicBlock &MBB) const {
  const uint64_t Size = MF.getFrameInfo().getStackSize();
  const bool HasFP = hasFP(MF);
  if (!Size && !HasFP)
    return;
  const SeaBirdSubtarget &STI = MF.getSubtarget<SeaBirdSubtarget>();
  const SeaBirdInstrInfo &TII = *STI.getInstrInfo();
  auto I = MBB.begin();
  const DebugLoc DL = I == MBB.end() ? DebugLoc() : I->getDebugLoc();
  const unsigned MOVI = STI.is64Bit() ? SeaBird::MOVI64 : SeaBird::MOVI32;
  const unsigned SUB = STI.is64Bit() ? SeaBird::SUBrr : SeaBird::SUBrr32;
  if (HasFP) {
    BuildMI(MBB, I, DL, TII.get(MOVI), SeaBird::R30)
        .addImm(16)
        .setMIFlag(MachineInstr::FrameSetup);
    BuildMI(MBB, I, DL, TII.get(SUB), SeaBird::R7)
        .addReg(SeaBird::R7)
        .addReg(SeaBird::R30)
        .setMIFlag(MachineInstr::FrameSetup);
    BuildMI(MBB, I, DL,
            TII.get(STI.is64Bit() ? SeaBird::STQrr : SeaBird::STWrr32))
        .addReg(SeaBird::R7)
        .addReg(SeaBird::NOIDX)
        .addImm(1)
        .addImm(0)
        .addReg(SeaBird::R6)
        .setMIFlag(MachineInstr::FrameSetup);
  }
  if (Size) {
    BuildMI(MBB, I, DL, TII.get(MOVI), SeaBird::R30)
        .addImm(Size)
        .setMIFlag(MachineInstr::FrameSetup);
    BuildMI(MBB, I, DL, TII.get(SUB), SeaBird::R7)
        .addReg(SeaBird::R7)
        .addReg(SeaBird::R30)
        .setMIFlag(MachineInstr::FrameSetup);
  }
  if (HasFP)
    BuildMI(MBB, I, DL,
            TII.get(STI.is64Bit() ? SeaBird::MOVrr : SeaBird::MOVrr32),
            SeaBird::R6)
        .addReg(SeaBird::R7)
        .setMIFlag(MachineInstr::FrameSetup);
}

void SeaBirdFrameLowering::emitEpilogue(MachineFunction &MF,
                                         MachineBasicBlock &MBB) const {
  const uint64_t Size = MF.getFrameInfo().getStackSize();
  const bool HasFP = hasFP(MF);
  if (!Size && !HasFP)
    return;
  const SeaBirdSubtarget &STI = MF.getSubtarget<SeaBirdSubtarget>();
  const SeaBirdInstrInfo &TII = *STI.getInstrInfo();
  auto I = MBB.getFirstTerminator();
  const DebugLoc DL = I == MBB.end() ? DebugLoc() : I->getDebugLoc();
  const unsigned MOVI = STI.is64Bit() ? SeaBird::MOVI64 : SeaBird::MOVI32;
  const unsigned ADD = STI.is64Bit() ? SeaBird::ADDrr : SeaBird::ADDrr32;
  if (HasFP)
    BuildMI(MBB, I, DL,
            TII.get(STI.is64Bit() ? SeaBird::MOVrr : SeaBird::MOVrr32),
            SeaBird::R7)
        .addReg(SeaBird::R6)
        .setMIFlag(MachineInstr::FrameDestroy);
  if (Size) {
    BuildMI(MBB, I, DL, TII.get(MOVI), SeaBird::R30)
        .addImm(Size)
        .setMIFlag(MachineInstr::FrameDestroy);
    BuildMI(MBB, I, DL, TII.get(ADD), SeaBird::R7)
        .addReg(SeaBird::R7)
        .addReg(SeaBird::R30)
        .setMIFlag(MachineInstr::FrameDestroy);
  }
  if (HasFP) {
    BuildMI(MBB, I, DL,
            TII.get(STI.is64Bit() ? SeaBird::LDQrr : SeaBird::LDWrr32),
            SeaBird::R6)
        .addReg(SeaBird::R7)
        .addReg(SeaBird::NOIDX)
        .addImm(1)
        .addImm(0)
        .setMIFlag(MachineInstr::FrameDestroy);
    BuildMI(MBB, I, DL, TII.get(MOVI), SeaBird::R30)
        .addImm(16)
        .setMIFlag(MachineInstr::FrameDestroy);
    BuildMI(MBB, I, DL, TII.get(ADD), SeaBird::R7)
        .addReg(SeaBird::R7)
        .addReg(SeaBird::R30)
        .setMIFlag(MachineInstr::FrameDestroy);
  }
}

void SeaBirdFrameLowering::determineCalleeSaves(
    MachineFunction &MF, BitVector &SavedRegs, RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);
  if (hasFP(MF))
    SavedRegs.reset(SeaBird::R6);
}

MachineBasicBlock::iterator SeaBirdFrameLowering::eliminateCallFramePseudoInstr(
    MachineFunction &MF, MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I) const {
  const uint64_t Size = I->getOperand(0).getImm();
  if (Size) {
    const SeaBirdSubtarget &STI = MF.getSubtarget<SeaBirdSubtarget>();
    const SeaBirdInstrInfo &TII = *STI.getInstrInfo();
    const DebugLoc DL = I->getDebugLoc();
    BuildMI(MBB, I, DL,
            TII.get(STI.is64Bit() ? SeaBird::MOVI64 : SeaBird::MOVI32),
            SeaBird::R30).addImm(Size);
    const unsigned Opcode = I->getOpcode() == TII.getCallFrameSetupOpcode()
                                ? (STI.is64Bit() ? SeaBird::SUBrr
                                               : SeaBird::SUBrr32)
                                : (STI.is64Bit() ? SeaBird::ADDrr
                                               : SeaBird::ADDrr32);
    BuildMI(MBB, I, DL, TII.get(Opcode), SeaBird::R7)
        .addReg(SeaBird::R7)
        .addReg(SeaBird::R30);
  }
  return MBB.erase(I);
}
