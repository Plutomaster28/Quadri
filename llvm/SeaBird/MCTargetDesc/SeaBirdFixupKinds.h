#ifndef LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDFIXUPKINDS_H
#define LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm::SeaBird {

enum Fixups {
  fixup_seabird_pcrel32 = FirstTargetFixupKind,
  fixup_seabird_tls_le,
  fixup_seabird_relative,
  fixup_seabird_jump_slot,
  fixup_seabird_glob_dat,
  NumTargetFixupKinds = fixup_seabird_glob_dat - FirstTargetFixupKind + 1,
};

} // namespace llvm::SeaBird

#endif
