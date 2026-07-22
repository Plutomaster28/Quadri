#include "SeaBirdTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

Target &llvm::getTheSeaBird32Target() {
  static Target Target;
  return Target;
}

Target &llvm::getTheSeaBird64Target() {
  static Target Target;
  return Target;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeSeaBirdTargetInfo() {
  RegisterTarget<Triple::seabird32> Y(getTheSeaBird32Target(), "seabird32",
                                      "SeaBird 32-bit", "SeaBird");
  RegisterTarget<Triple::seabird64> X(getTheSeaBird64Target(), "seabird64",
                                      "SeaBird 64-bit", "SeaBird");
}
