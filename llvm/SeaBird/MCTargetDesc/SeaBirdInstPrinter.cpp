#include "SeaBirdInstPrinter.h"
#include "SeaBirdMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#include "SeaBirdGenAsmWriter.inc"

void SeaBirdInstPrinter::printRegName(raw_ostream &OS, MCRegister Reg) {
  OS << getRegisterName(Reg);
}

void SeaBirdInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                      raw_ostream &OS) {
  const MCOperand &Operand = MI->getOperand(OpNo);
  if (Operand.isReg()) {
    printRegName(OS, Operand.getReg());
  } else if (Operand.isImm()) {
    OS << Operand.getImm();
  } else {
    MAI.printExpr(OS, *Operand.getExpr());
  }
}

void SeaBirdInstPrinter::printMemoryOperand(const MCInst *MI, unsigned OpNo,
                                            raw_ostream &OS) {
  OS << '[';
  printRegName(OS, MI->getOperand(OpNo).getReg());
  const MCRegister Index = MI->getOperand(OpNo + 1).getReg();
  const std::int64_t Scale = MI->getOperand(OpNo + 2).getImm();
  const std::int64_t Disp = MI->getOperand(OpNo + 3).getImm();
  if (Index != SeaBird::R4) {
    OS << " + ";
    printRegName(OS, Index);
    if (Scale != 1)
      OS << '*' << Scale;
  }
  if (Disp > 0)
    OS << " + " << Disp;
  else if (Disp < 0)
    OS << " - " << -Disp;
  OS << ']';
}

void SeaBirdInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                   StringRef Annotation,
                                   const MCSubtargetInfo &STI,
                                   raw_ostream &OS) {
  printInstruction(MI, Address, OS);
  printAnnotation(OS, Annotation);
}
