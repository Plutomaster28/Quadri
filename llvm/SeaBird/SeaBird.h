#ifndef LLVM_LIB_TARGET_SEABIRD_SEABIRD_H
#define LLVM_LIB_TARGET_SEABIRD_SEABIRD_H

#include "llvm/Pass.h"

namespace llvm {

class FunctionPass;
class PassRegistry;
class SeaBirdTargetMachine;

FunctionPass *createSeaBirdISelDag(SeaBirdTargetMachine &TM);
void initializeSeaBirdAsmPrinterPass(PassRegistry &Registry);
void initializeSeaBirdDAGToDAGISelLegacyPass(PassRegistry &Registry);

} // namespace llvm

#endif
