#include "SeaBirdRegisterInfo.h"
#include "SeaBirdFrameLowering.h"
#include "SeaBirdInstrInfo.h"
#include "SeaBirdSubtarget.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_REGINFO_TARGET_DESC
#include "SeaBirdGenRegisterInfo.inc"

using namespace llvm;

SeaBirdRegisterInfo::SeaBirdRegisterInfo() : SeaBirdGenRegisterInfo(0) {}

const std::uint16_t *
SeaBirdRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const {
  if (MF && MF->getSubtarget<SeaBirdSubtarget>().hasRegisterWindows())
    return CSR_SeaBirdWindowSpills_SaveList;
  return CSR_SeaBird_SaveList;
}

const std::uint32_t *SeaBirdRegisterInfo::getCallPreservedMask(
    const MachineFunction &MF, CallingConv::ID CC) const {
  if (MF.getSubtarget<SeaBirdSubtarget>().hasRegisterWindows())
    return CSR_SeaBirdWindowPreserved_RegMask;
  return CSR_SeaBird_RegMask;
}

BitVector SeaBirdRegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  BitVector Reserved(getNumRegs());
  Reserved.set(SeaBird::R6); // Frame pointer until frame lowering is enabled.
  Reserved.set(SeaBird::R7); // Stack pointer.
  Reserved.set(SeaBird::NOIDX); // Non-architectural no-index encoding sentinel.
  if (!MF.getSubtarget<SeaBirdSubtarget>().hasRegisterWindows()) {
    Reserved.set(SeaBird::R28);
    Reserved.set(SeaBird::R29);
    Reserved.set(SeaBird::R30);
    Reserved.set(SeaBird::R31);
  }
  return Reserved;
}

bool SeaBirdRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator MI,
                                               int SPAdj,
                                               unsigned FIOperandNum,
                                               RegScavenger *RS) const {
  MachineFunction &MF = *MI->getParent()->getParent();
  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const int FrameIndex = MI->getOperand(FIOperandNum).getIndex();
  const int64_t Offset = MFI.getObjectOffset(FrameIndex) + MFI.getStackSize();
  const SeaBirdInstrInfo &TII = *MF.getSubtarget<SeaBirdSubtarget>().getInstrInfo();
  const bool Is64Bit = MF.getSubtarget<SeaBirdSubtarget>().is64Bit();
  const Register FrameReg = getFrameRegister(MF);
  MachineBasicBlock &MBB = *MI->getParent();
  const DebugLoc DL = MI->getDebugLoc();

  if (Offset == 0) {
    MI->getOperand(FIOperandNum).ChangeToRegister(FrameReg, false);
    return false;
  }
  BuildMI(MBB, MI, DL,
          TII.get(Is64Bit ? SeaBird::MOVI64 : SeaBird::MOVI32),
          SeaBird::R30).addImm(Offset);
  BuildMI(MBB, MI, DL,
          TII.get(Is64Bit ? SeaBird::ADDrr : SeaBird::ADDrr32),
          SeaBird::R30)
      .addReg(SeaBird::R30)
      .addReg(FrameReg);
  MI->getOperand(FIOperandNum).ChangeToRegister(SeaBird::R30, false);
  return false;
}

Register
SeaBirdRegisterInfo::getFrameRegister(const MachineFunction &MF) const {
  return MF.getSubtarget<SeaBirdSubtarget>().getFrameLowering()->hasFP(MF)
             ? SeaBird::R6
             : SeaBird::R7;
}
