#include "SeaBirdELFRelocs.h"
#include "SeaBirdFixupKinds.h"
#include "SeaBirdMCTargetDesc.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {

class SeaBirdELFObjectWriter final : public MCELFObjectTargetWriter {
public:
  SeaBirdELFObjectWriter(std::uint8_t OSABI, bool Is64Bit)
      : MCELFObjectTargetWriter(Is64Bit, OSABI, SeaBirdELF::EM_SEABIRD, true) {}

  unsigned getRelocType(const MCFixup &Fixup, const MCValue &Target,
                        bool IsPCRel) const override {
    switch (Fixup.getKind()) {
    case FK_Data_2:
      return SeaBirdELF::R_SB_ABS16;
    case FK_Data_4:
      return SeaBirdELF::R_SB_ABS32;
    case FK_Data_8:
      return SeaBirdELF::R_SB_ABS64;
    case SeaBird::fixup_seabird_pcrel32:
      return SeaBirdELF::R_SB_PCREL32;
    case SeaBird::fixup_seabird_tls_le:
      return SeaBirdELF::R_SB_TLS_LE;
    case SeaBird::fixup_seabird_relative:
      return SeaBirdELF::R_SB_RELATIVE;
    case SeaBird::fixup_seabird_jump_slot:
      return SeaBirdELF::R_SB_JUMP_SLOT;
    case SeaBird::fixup_seabird_glob_dat:
      return SeaBirdELF::R_SB_GLOB_DAT;
    default:
      llvm_unreachable("unsupported SeaBird ELF relocation");
    }
  }
};

} // namespace

std::unique_ptr<MCObjectTargetWriter>
llvm::createSeaBirdELFObjectWriter(std::uint8_t OSABI, bool Is64Bit) {
  return std::make_unique<SeaBirdELFObjectWriter>(OSABI, Is64Bit);
}
