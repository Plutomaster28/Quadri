//===--- SeaBird.cpp - Implement SeaBird target feature support -----------===//

#include "SeaBird.h"
#include "clang/Basic/MacroBuilder.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;
using namespace clang::targets;

SeaBirdTargetInfo::SeaBirdTargetInfo(const llvm::Triple &Triple,
                                     const TargetOptions &Opts, bool Is64Bit)
    : TargetInfo(Triple), Is64Bit(Is64Bit) {
  NoAsmVariants = true;
  TLSSupported = true;

  IntWidth = IntAlign = 32;
  LongLongWidth = LongLongAlign = 64;
  FloatWidth = FloatAlign = 32;
  DoubleWidth = DoubleAlign = 64;
  LongDoubleWidth = LongDoubleAlign = 128;
  LongDoubleFormat = &llvm::APFloat::IEEEquad();
  SuitableAlign = 128;
  WCharType = SignedInt;
  WIntType = UnsignedInt;
  UseZeroLengthBitfieldAlignment = true;

  if (Is64Bit) {
    LongWidth = LongAlign = 64;
    PointerWidth = PointerAlign = 64;
    SizeType = UnsignedLong;
    PtrDiffType = SignedLong;
    IntPtrType = SignedLong;
    IntMaxType = SignedLong;
    MaxAtomicPromoteWidth = MaxAtomicInlineWidth = 64;
  } else {
    LongWidth = LongAlign = 32;
    PointerWidth = PointerAlign = 32;
    SizeType = UnsignedInt;
    PtrDiffType = SignedInt;
    IntPtrType = SignedInt;
    IntMaxType = SignedLongLong;
    MaxAtomicPromoteWidth = MaxAtomicInlineWidth = 32;
  }
  resetDataLayout();
}

void SeaBirdTargetInfo::getTargetDefines(const LangOptions &Opts,
                                         MacroBuilder &Builder) const {
  Builder.defineMacro("__seabird__");
  Builder.defineMacro("__SEABIRD__");
  if (Is64Bit) {
    Builder.defineMacro("__seabird64__");
    Builder.defineMacro("__SEABIRD_DRAGONET__");
  } else {
    Builder.defineMacro("__seabird32__");
    Builder.defineMacro("__SEABIRD_TETRA__");
  }
  if (CPU == "tritium-v1")
    Builder.defineMacro("__SEABIRD_TRITIUM__");
}

bool SeaBirdTargetInfo::hasFeature(StringRef Feature) const {
  return llvm::StringSwitch<bool>(Feature)
      .Case("seabird", true)
      .Case("seabird32", !Is64Bit)
      .Case("seabird64", Is64Bit)
      .Case("tritium", CPU == "tritium-v1")
      .Default(false);
}

bool SeaBirdTargetInfo::isValidCPUName(StringRef Name) const {
  if (Name == "tritium-v1")
    return !Is64Bit;
  return Name == "generic" || Name == "seabird-gold" ||
         Name == "seabird-platinum";
}

void SeaBirdTargetInfo::fillValidCPUList(
    SmallVectorImpl<StringRef> &Values) const {
  Values.append({"generic", "seabird-gold", "seabird-platinum"});
  if (!Is64Bit)
    Values.push_back("tritium-v1");
}

bool SeaBirdTargetInfo::setCPU(const std::string &Name) {
  if (!isValidCPUName(Name))
    return false;
  CPU = Name;
  return true;
}

ArrayRef<const char *> SeaBirdTargetInfo::getGCCRegNames() const {
  static const char *const Names[] = {
      "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
      "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
      "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
      "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
      "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
      "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
      "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"};
  return llvm::ArrayRef(Names);
}

ArrayRef<TargetInfo::GCCRegAlias>
SeaBirdTargetInfo::getGCCRegAliases() const {
  static const TargetInfo::GCCRegAlias Aliases[] = {
      {{"sp"}, "r7"}, {{"fp", "bp"}, "r6"}, {{"lr"}, "r27"}};
  return llvm::ArrayRef(Aliases);
}

bool SeaBirdTargetInfo::validateAsmConstraint(
    const char *&Name, TargetInfo::ConstraintInfo &Info) const {
  switch (*Name) {
  case 'r':
  case 'v':
    Info.setAllowsRegister();
    return true;
  case 'I':
    Info.setRequiresImmediate();
    return true;
  default:
    return false;
  }
}
