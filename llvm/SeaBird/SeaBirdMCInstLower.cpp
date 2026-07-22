#include "SeaBirdMCInstLower.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

void SeaBirdMCInstLower::lower(const MachineInstr *MI, MCInst &Out) const {
  Out.setOpcode(MI->getOpcode());
  for (const MachineOperand &MO : MI->operands()) {
    if (MO.isImplicit() || MO.isRegMask())
      continue;
    if (MO.isReg()) {
      Out.addOperand(MCOperand::createReg(MO.getReg()));
    } else if (MO.isImm()) {
      Out.addOperand(MCOperand::createImm(MO.getImm()));
    } else if (MO.isMBB()) {
      Out.addOperand(MCOperand::createExpr(
          MCSymbolRefExpr::create(MO.getMBB()->getSymbol(), Context)));
    } else if (MO.isGlobal()) {
      const MCExpr *Expr =
          MCSymbolRefExpr::create(Printer.getSymbol(MO.getGlobal()), Context);
      if (MO.getOffset())
        Expr = MCBinaryExpr::createAdd(
            Expr, MCConstantExpr::create(MO.getOffset(), Context), Context);
      Out.addOperand(MCOperand::createExpr(Expr));
    } else if (MO.isSymbol()) {
      Out.addOperand(MCOperand::createExpr(MCSymbolRefExpr::create(
          Printer.GetExternalSymbolSymbol(MO.getSymbolName()), Context)));
    } else if (MO.isCPI()) {
      const MCExpr *Expr = MCSymbolRefExpr::create(
          Printer.GetCPISymbol(MO.getIndex()), Context);
      if (MO.getOffset())
        Expr = MCBinaryExpr::createAdd(
            Expr, MCConstantExpr::create(MO.getOffset(), Context), Context);
      Out.addOperand(MCOperand::createExpr(Expr));
    } else {
      MI->print(errs());
      llvm_unreachable("unsupported SeaBird machine operand");
    }
  }
}
