#include "SeaBirdSubtarget.h"

#define DEBUG_TYPE "seabird-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "SeaBirdGenSubtargetInfo.inc"

using namespace llvm;

SeaBirdSubtarget &SeaBirdSubtarget::initializeSubtargetDependencies(
    StringRef CPU, StringRef Features) {
  std::string CPUName = CPU.empty() ? "generic" : CPU.str();
  ParseSubtargetFeatures(CPUName, CPUName, Features);
  return *this;
}

SeaBirdSubtarget::SeaBirdSubtarget(const Triple &TT, StringRef CPU,
                                   StringRef Features,
                                   const TargetMachine &TM)
    : SeaBirdGenSubtargetInfo(TT, CPU, CPU, Features),
      Is64Bit(TT.isArch64Bit()),
      InstrInfo(initializeSubtargetDependencies(CPU, Features)),
      FrameLowering(), TLInfo(TM, *this) {}
