#ifndef LLVM_LIB_TARGET_SEABIRD_SEABIRDMCINSTLOWER_H
#define LLVM_LIB_TARGET_SEABIRD_SEABIRDMCINSTLOWER_H

namespace llvm {

class AsmPrinter;
class MCContext;
class MCInst;
class MachineInstr;

class SeaBirdMCInstLower {
  MCContext &Context;
  AsmPrinter &Printer;

public:
  SeaBirdMCInstLower(MCContext &Context, AsmPrinter &Printer)
      : Context(Context), Printer(Printer) {}
  void lower(const MachineInstr *MI, MCInst &Out) const;
};

} // namespace llvm

#endif
