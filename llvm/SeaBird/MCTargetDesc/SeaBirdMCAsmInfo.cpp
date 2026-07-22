#include "SeaBirdMCAsmInfo.h"
#include "llvm/TargetParser/Triple.h"

using namespace llvm;

void SeaBirdMCAsmInfo::anchor() {}

SeaBirdMCAsmInfo::SeaBirdMCAsmInfo(const Triple &TT,
                                   const MCTargetOptions &Options) {
  IsLittleEndian = true;
  CodePointerSize = TT.isArch64Bit() ? 8 : 4;
  CalleeSaveStackSlotSize = CodePointerSize;
  CommentString = ";";
  PrivateGlobalPrefix = ".L";
  ExceptionsType = ExceptionHandling::DwarfCFI;
  MinInstAlignment = 1;
  MaxInstLength = 13;
  SupportsDebugInformation = true;
  UsesELFSectionDirectiveForBSS = true;
}
