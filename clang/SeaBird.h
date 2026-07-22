//===--- SeaBird.h - Declare SeaBird target feature support -----*- C++ -*-===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_SEABIRD_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_SEABIRD_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY SeaBirdTargetInfo : public TargetInfo {
protected:
  std::string CPU = "generic";
  bool Is64Bit;

  SeaBirdTargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts,
                    bool Is64Bit);

public:
  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override {
    return {};
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  bool hasFeature(StringRef Feature) const override;
  bool isValidCPUName(StringRef Name) const override;
  void fillValidCPUList(SmallVectorImpl<StringRef> &Values) const override;
  bool setCPU(const std::string &Name) override;

  ArrayRef<const char *> getGCCRegNames() const override;
  ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override;

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override;
  std::string_view getClobbers() const override { return ""; }

  bool hasBitIntType() const override { return true; }
  bool isCLZForZeroUndef() const override { return false; }
};

class LLVM_LIBRARY_VISIBILITY SeaBird32TargetInfo final
    : public SeaBirdTargetInfo {
public:
  SeaBird32TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : SeaBirdTargetInfo(Triple, Opts, false) {}
};

class LLVM_LIBRARY_VISIBILITY SeaBird64TargetInfo final
    : public SeaBirdTargetInfo {
public:
  SeaBird64TargetInfo(const llvm::Triple &Triple, const TargetOptions &Opts)
      : SeaBirdTargetInfo(Triple, Opts, true) {}
};

} // namespace targets
} // namespace clang

#endif
