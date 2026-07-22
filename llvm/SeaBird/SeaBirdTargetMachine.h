#ifndef LLVM_LIB_TARGET_SEABIRD_SEABIRDTARGETMACHINE_H
#define LLVM_LIB_TARGET_SEABIRD_SEABIRDTARGETMACHINE_H

#include "SeaBirdSubtarget.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"

#include <memory>
#include <optional>

namespace llvm {

class SeaBirdTargetMachine final : public CodeGenTargetMachineImpl {
  SeaBirdSubtarget Subtarget;
  std::unique_ptr<TargetLoweringObjectFile> TLOF;

public:
  SeaBirdTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef Features, const TargetOptions &Options,
                       std::optional<Reloc::Model> RM,
                       std::optional<CodeModel::Model> CM,
                       CodeGenOptLevel OL, bool JIT);

  const SeaBirdSubtarget *
  getSubtargetImpl(const Function &F) const override {
    return &Subtarget;
  }
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  MachineFunctionInfo *
  createMachineFunctionInfo(BumpPtrAllocator &Allocator, const Function &F,
                            const TargetSubtargetInfo *STI) const override;
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
  bool isMachineVerifierClean() const override { return false; }
};

} // namespace llvm

#endif
