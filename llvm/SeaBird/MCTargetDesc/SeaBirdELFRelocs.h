#ifndef LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDELFRELOCS_H
#define LLVM_LIB_TARGET_SEABIRD_MCTARGETDESC_SEABIRDELFRELOCS_H

#include <cstdint>

namespace llvm::SeaBirdELF {

constexpr std::uint16_t EM_SEABIRD = 0x5342;
constexpr unsigned EF_SB_WINDOWED_ABI = 1U << 0;
constexpr unsigned EF_SB_PAE32_REQUIRED = 1U << 1;
enum Relocation : unsigned {
  R_SB_NONE = 0,
  R_SB_ABS16 = 1,
  R_SB_ABS32 = 2,
  R_SB_ABS64 = 3,
  R_SB_ABS128 = 4,
  R_SB_PCREL8 = 5,
  R_SB_PCREL16 = 6,
  R_SB_PCREL32 = 7,
  R_SB_TLS_LE = 13,
  R_SB_RELATIVE = 14,
  R_SB_JUMP_SLOT = 15,
  R_SB_GLOB_DAT = 16,
};

} // namespace llvm::SeaBirdELF

#endif
