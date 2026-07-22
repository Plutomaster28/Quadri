#ifndef LLVM_LIB_TARGET_SEABIRD_TARGETINFO_SEABIRDTARGETINFO_H
#define LLVM_LIB_TARGET_SEABIRD_TARGETINFO_SEABIRDTARGETINFO_H

namespace llvm {

class Target;

Target &getTheSeaBird32Target();
Target &getTheSeaBird64Target();

} // namespace llvm

#endif
