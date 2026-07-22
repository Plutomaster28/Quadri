#ifndef LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDMCTARGETDESC_H
#define LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDMCTARGETDESC_H

#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCTargetOptions.h"

#include <cstdint>
#include <memory>

namespace llvm {

class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCSubtargetInfo;
class Target;

MCCodeEmitter *createSeaBirdMCCodeEmitter(const MCInstrInfo &MCII,
                                          MCContext &Context);
MCAsmBackend *createSeaBirdAsmBackend(const Target &T,
                                      const MCSubtargetInfo &STI,
                                      const MCRegisterInfo &MRI,
                                      const MCTargetOptions &Options);
std::unique_ptr<MCObjectTargetWriter>
createSeaBirdELFObjectWriter(std::uint8_t OSABI, bool Is64Bit);

} // namespace llvm

#define GET_REGINFO_ENUM
#include "SeaBirdGenRegisterInfo.inc"

#define GET_INSTRINFO_ENUM
#include "SeaBirdGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "SeaBirdGenSubtargetInfo.inc"

#endif
