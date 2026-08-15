#include "SeaBirdInstPrinter.h"
#include "SeaBirdBaseInfo.h"
#include "SeaBirdMCTargetDesc.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/ADT/SmallString.h"
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
  if (Index != SeaBird::R4 && Index != SeaBird::NOIDX) {
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
  const unsigned Marker = MI->getFlags() & SeaBirdII::MarkerMask;
  switch (Marker) {
  case SeaBirdII::Assume: OS << "assume."; break;
  case SeaBirdII::Likely: OS << "likely."; break;
  case SeaBirdII::Unlikely: OS << "unlikely."; break;
  case SeaBirdII::Stream: OS << "stream."; break;
  case SeaBirdII::Prefetch: OS << "prefetch."; break;
  case SeaBirdII::Temporary: OS << "temporary."; break;
  case SeaBirdII::Persistent: OS << "persistent."; break;
  case SeaBirdII::Independent: OS << "independent."; break;
  case SeaBirdII::Reuse: OS << "reuse."; break;
  case SeaBirdII::Leaf: OS << "leaf."; break;
  default: break;
  }
  if (Marker == SeaBirdII::NoMarker) {
    printInstruction(MI, Address, OS);
  } else {
    SmallString<128> Text;
    raw_svector_ostream Stream(Text);
    printInstruction(MI, Address, Stream);
    OS << StringRef(Text).ltrim();
  }
  printAnnotation(OS, Annotation);
}
