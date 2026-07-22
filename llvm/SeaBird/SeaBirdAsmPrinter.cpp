#include "SeaBird.h"
#include "SeaBirdMCInstLower.h"
#include "MCTargetDesc/SeaBirdMCTargetDesc.h"
#include "TargetInfo/SeaBirdTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

namespace {

class SeaBirdAsmPrinter final : public AsmPrinter {
public:
  static char ID;
  SeaBirdAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}

  StringRef getPassName() const override { return "SeaBird Assembly Printer"; }

  void emitInstruction(const MachineInstr *MI) override {
    if (MI->getOpcode() == SeaBird::ADJCALLSTACKDOWN ||
        MI->getOpcode() == SeaBird::ADJCALLSTACKUP) {
      const int64_t Size = MI->getOperand(0).getImm();
      if (!Size)
        return;
      const bool Is64Bit = TM.getTargetTriple().isArch64Bit();
      MCInst Amount;
      Amount.setOpcode(Is64Bit ? SeaBird::MOVI64 : SeaBird::MOVI32);
      Amount.addOperand(MCOperand::createReg(SeaBird::R30));
      Amount.addOperand(MCOperand::createImm(Size));
      OutStreamer->emitInstruction(Amount, getSubtargetInfo());
      MCInst Adjust;
      Adjust.setOpcode(
          MI->getOpcode() == SeaBird::ADJCALLSTACKDOWN
              ? (Is64Bit ? SeaBird::SUBrr : SeaBird::SUBrr32)
              : (Is64Bit ? SeaBird::ADDrr : SeaBird::ADDrr32));
      Adjust.addOperand(MCOperand::createReg(SeaBird::R7));
      Adjust.addOperand(MCOperand::createReg(SeaBird::R7));
      Adjust.addOperand(MCOperand::createReg(SeaBird::R30));
      OutStreamer->emitInstruction(Adjust, getSubtargetInfo());
      return;
    }
    SeaBirdMCInstLower Lowering(OutContext, *this);
    MCInst Inst;
    Lowering.lower(MI, Inst);
    OutStreamer->emitInstruction(Inst, getSubtargetInfo());
  }
};

} // namespace

char SeaBirdAsmPrinter::ID = 0;

INITIALIZE_PASS(SeaBirdAsmPrinter, "seabird-asm-printer",
                "SeaBird Assembly Printer", false, false)

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeSeaBirdAsmPrinter() {
  RegisterAsmPrinter<SeaBirdAsmPrinter> X32(getTheSeaBird32Target());
  RegisterAsmPrinter<SeaBirdAsmPrinter> X64(getTheSeaBird64Target());
}
