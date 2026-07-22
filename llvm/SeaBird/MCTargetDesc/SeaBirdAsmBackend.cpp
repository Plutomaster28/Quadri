#include "SeaBirdFixupKinds.h"
#include "SeaBirdMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"

using namespace llvm;

namespace {

class SeaBirdAsmBackend final : public MCAsmBackend {
  Triple::OSType OSType;
  bool Is64Bit;

public:
  explicit SeaBirdAsmBackend(const Triple &TT)
      : MCAsmBackend(endianness::little), OSType(TT.getOS()),
        Is64Bit(TT.isArch64Bit()) {}

  std::optional<MCFixupKind> getFixupKind(StringRef Name) const override {
    if (Name == "R_SB_TLS_LE")
      return MCFixupKind(SeaBird::fixup_seabird_tls_le);
    if (Name == "R_SB_RELATIVE")
      return MCFixupKind(SeaBird::fixup_seabird_relative);
    if (Name == "R_SB_JUMP_SLOT")
      return MCFixupKind(SeaBird::fixup_seabird_jump_slot);
    if (Name == "R_SB_GLOB_DAT")
      return MCFixupKind(SeaBird::fixup_seabird_glob_dat);
    return std::nullopt;
  }

  void applyFixup(const MCFragment &Fragment, const MCFixup &Fixup,
                  const MCValue &Target, std::uint8_t *Data,
                  std::uint64_t Value, bool IsResolved) override {
    if (!IsResolved)
      Asm->getWriter().recordRelocation(Fragment, Fixup, Target, Value);

    unsigned Bytes = 0;
    switch (Fixup.getKind()) {
    case FK_Data_2:
      Bytes = 2;
      break;
    case FK_Data_4:
      Bytes = 4;
      break;
    case FK_Data_8:
      Bytes = 8;
      break;
    case SeaBird::fixup_seabird_pcrel32:
      Bytes = 4;
      break;
    case SeaBird::fixup_seabird_tls_le:
    case SeaBird::fixup_seabird_relative:
    case SeaBird::fixup_seabird_jump_slot:
    case SeaBird::fixup_seabird_glob_dat:
      Bytes = Is64Bit ? 8 : 4;
      break;
    default:
      return;
    }
    assert(Fixup.getOffset() + Bytes <= Fragment.getSize() &&
           "invalid SeaBird fixup offset");
    for (unsigned I = 0; I < Bytes; ++I)
      Data[I] |= static_cast<std::uint8_t>(Value >> (I * 8));
  }

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override {
    return createSeaBirdELFObjectWriter(
        MCELFObjectTargetWriter::getOSABI(OSType), Is64Bit);
  }

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override {
    if (Kind < FirstTargetFixupKind)
      return MCAsmBackend::getFixupKindInfo(Kind);
    const std::uint8_t TargetBits = Is64Bit ? 64 : 32;
    if (Kind == SeaBird::fixup_seabird_pcrel32)
      return {"fixup_seabird_pcrel32", 0, 32, 0};
    if (Kind == SeaBird::fixup_seabird_tls_le)
      return {"fixup_seabird_tls_le", 0, TargetBits, 0};
    if (Kind == SeaBird::fixup_seabird_relative)
      return {"fixup_seabird_relative", 0, TargetBits, 0};
    if (Kind == SeaBird::fixup_seabird_jump_slot)
      return {"fixup_seabird_jump_slot", 0, TargetBits, 0};
    assert(Kind == SeaBird::fixup_seabird_glob_dat);
    return {"fixup_seabird_glob_dat", 0, TargetBits, 0};
  }

  bool writeNopData(raw_ostream &OS, std::uint64_t Count,
                    const MCSubtargetInfo *STI) const override {
    return Count == 0;
  }
};

} // namespace

MCAsmBackend *llvm::createSeaBirdAsmBackend(
    const Target &T, const MCSubtargetInfo &STI, const MCRegisterInfo &MRI,
    const MCTargetOptions &Options) {
  return new SeaBirdAsmBackend(STI.getTargetTriple());
}
