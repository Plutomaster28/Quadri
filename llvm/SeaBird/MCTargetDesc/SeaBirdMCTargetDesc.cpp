#include "SeaBirdMCTargetDesc.h"
#include "SeaBirdInstPrinter.h"
#include "SeaBirdMCAsmInfo.h"
#include "TargetInfo/SeaBirdTargetInfo.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

#include <string>

#define GET_INSTRINFO_MC_DESC
#include "SeaBirdGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "SeaBirdGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "SeaBirdGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createSeaBirdMCInstrInfo() {
  auto *Info = new MCInstrInfo();
  InitSeaBirdMCInstrInfo(Info);
  return Info;
}

static MCRegisterInfo *createSeaBirdMCRegisterInfo(const Triple &TT) {
  auto *Info = new MCRegisterInfo();
  InitSeaBirdMCRegisterInfo(Info, SeaBird::R27);
  return Info;
}

static MCSubtargetInfo *createSeaBirdMCSubtargetInfo(const Triple &TT,
                                                     StringRef CPU,
                                                     StringRef Features) {
  std::string CPUName = CPU.empty() ? "generic" : CPU.str();
  return createSeaBirdMCSubtargetInfoImpl(TT, CPUName, CPUName, Features);
}

static MCInstPrinter *createSeaBirdMCInstPrinter(
    const Triple &TT, unsigned SyntaxVariant, const MCAsmInfo &MAI,
    const MCInstrInfo &MII, const MCRegisterInfo &MRI) {
  if (SyntaxVariant != 0)
    return nullptr;
  return new SeaBirdInstPrinter(MAI, MII, MRI);
}

static MCStreamer *createSeaBirdELFStreamer(
    const Triple &TT, MCContext &Context, std::unique_ptr<MCAsmBackend> &&Backend,
    std::unique_ptr<MCObjectWriter> &&Writer,
    std::unique_ptr<MCCodeEmitter> &&Emitter) {
  return createELFStreamer(Context, std::move(Backend), std::move(Writer),
                           std::move(Emitter));
}

static MCRelocationInfo *createSeaBirdRelocationInfo(const Triple &TT,
                                                      MCContext &Context) {
  return createMCRelocationInfo(TT, Context);
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeSeaBirdTargetMC() {
  for (Target *T : {&getTheSeaBird32Target(), &getTheSeaBird64Target()}) {
    RegisterMCAsmInfo<SeaBirdMCAsmInfo> AsmInfo(*T);
    TargetRegistry::RegisterMCInstrInfo(*T, createSeaBirdMCInstrInfo);
    TargetRegistry::RegisterMCRegInfo(*T, createSeaBirdMCRegisterInfo);
    TargetRegistry::RegisterMCSubtargetInfo(*T,
                                            createSeaBirdMCSubtargetInfo);
    TargetRegistry::RegisterMCInstPrinter(*T, createSeaBirdMCInstPrinter);
    TargetRegistry::RegisterMCCodeEmitter(*T, createSeaBirdMCCodeEmitter);
    TargetRegistry::RegisterMCAsmBackend(*T, createSeaBirdAsmBackend);
    TargetRegistry::RegisterELFStreamer(*T, createSeaBirdELFStreamer);
    TargetRegistry::RegisterMCRelocationInfo(*T,
                                             createSeaBirdRelocationInfo);
  }
}
