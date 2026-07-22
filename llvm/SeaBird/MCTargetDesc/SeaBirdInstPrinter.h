#ifndef LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDINSTPRINTER_H
#define LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDINSTPRINTER_H

#include "llvm/MC/MCInstPrinter.h"

namespace llvm {

class SeaBirdInstPrinter : public MCInstPrinter {
public:
  SeaBirdInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                     const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printInst(const MCInst *MI, uint64_t Address, StringRef Annotation,
                 const MCSubtargetInfo &STI, raw_ostream &OS) override;
  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &OS);
  void printMemoryOperand(const MCInst *MI, unsigned OpNo, raw_ostream &OS);
  void printRegName(raw_ostream &OS, MCRegister Reg) override;

  std::pair<const char *, uint64_t>
  getMnemonic(const MCInst &MI) const override;
  void printInstruction(const MCInst *MI, uint64_t Address, raw_ostream &OS);
  static const char *getRegisterName(MCRegister Reg);
};

} // namespace llvm

#endif
