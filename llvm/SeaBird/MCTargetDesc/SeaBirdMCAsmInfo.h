#ifndef LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDMCASMINFO_H
#define LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {

class Triple;

class SeaBirdMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit SeaBirdMCAsmInfo(const Triple &TT, const MCTargetOptions &Options);
};

} // namespace llvm

#endif
