#ifndef LLVM_LIB_TARGET_SEABIRD_SEABIRDSUBTARGET_H
#define LLVM_LIB_TARGET_SEABIRD_SEABIRDSUBTARGET_H

#include "SeaBirdFrameLowering.h"
#include "SeaBirdISelLowering.h"
#include "SeaBirdInstrInfo.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "SeaBirdGenSubtargetInfo.inc"

namespace llvm {

class SeaBirdSubtarget final : public SeaBirdGenSubtargetInfo {
  bool Is64Bit;
  bool HasTritium = false;
  bool HasTetra = false;
  bool HasMPU = false;
  bool HasAtomics = false;
  bool HasFixedMulDiv = false;
  SeaBirdInstrInfo InstrInfo;
  SeaBirdFrameLowering FrameLowering;
  SeaBirdTargetLowering TLInfo;
  SelectionDAGTargetInfo TSInfo;

  SeaBirdSubtarget &initializeSubtargetDependencies(StringRef CPU,
                                                     StringRef Features);

public:
  SeaBirdSubtarget(const Triple &TT, StringRef CPU, StringRef Features,
                   const TargetMachine &TM);

  bool is64Bit() const { return Is64Bit; }
  bool hasTritium() const { return HasTritium; }
  bool hasTetra() const { return HasTetra; }
  bool hasMPU() const { return HasMPU; }
  bool hasAtomics() const { return HasAtomics; }
  bool hasFixedMulDiv() const { return HasFixedMulDiv; }
  MVT getScalarVT() const { return Is64Bit ? MVT::i64 : MVT::i32; }
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);
  const SeaBirdInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const TargetFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const SeaBirdRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const SeaBirdTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
};

} // namespace llvm

#endif
