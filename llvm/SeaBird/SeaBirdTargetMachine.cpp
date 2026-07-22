#include "SeaBirdTargetMachine.h"
#include "SeaBird.h"
#include "SeaBirdMachineFunctionInfo.h"
#include "TargetInfo/SeaBirdTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeSeaBirdTarget() {
  RegisterTargetMachine<SeaBirdTargetMachine> X32(getTheSeaBird32Target());
  RegisterTargetMachine<SeaBirdTargetMachine> X64(getTheSeaBird64Target());
  PassRegistry &Registry = *PassRegistry::getPassRegistry();
  initializeSeaBirdAsmPrinterPass(Registry);
  initializeSeaBirdDAGToDAGISelLegacyPass(Registry);
}

static Reloc::Model effectiveRelocModel(std::optional<Reloc::Model> RM) {
  return RM.value_or(Reloc::Static);
}

SeaBirdTargetMachine::SeaBirdTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef Features,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL, bool JIT)
    : CodeGenTargetMachineImpl(
          T, TT.computeDataLayout(""), TT, CPU, Features, Options,
          effectiveRelocModel(RM),
          getEffectiveCodeModel(CM, CodeModel::Small), OL),
      Subtarget(TT, CPU, Features, *this),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()) {
  initAsmInfo();
}

namespace {

class SeaBirdPassConfig final : public TargetPassConfig {
public:
  SeaBirdPassConfig(SeaBirdTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}

  bool addInstSelector() override {
    addPass(createSeaBirdISelDag(getTM<SeaBirdTargetMachine>()));
    return false;
  }
};

} // namespace

TargetPassConfig *
SeaBirdTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new SeaBirdPassConfig(*this, PM);
}

MachineFunctionInfo *SeaBirdTargetMachine::createMachineFunctionInfo(
    BumpPtrAllocator &Allocator, const Function &F,
    const TargetSubtargetInfo *STI) const {
  return SeaBirdMachineFunctionInfo::create<SeaBirdMachineFunctionInfo>(
      Allocator, F, STI);
}
