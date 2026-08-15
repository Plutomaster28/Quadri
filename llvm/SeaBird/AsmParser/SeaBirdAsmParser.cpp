#include "MCTargetDesc/SeaBirdMCTargetDesc.h"
#include "MCTargetDesc/SeaBirdBaseInfo.h"
#include "TargetInfo/SeaBirdTargetInfo.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/SMLoc.h"
#include "llvm/Support/SourceMgr.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

using namespace llvm;

static MCRegister MatchRegisterName(StringRef Name);

namespace {

class SeaBirdOperand : public MCParsedAsmOperand {
  enum KindTy { Token, Register, Immediate, Memory } Kind;
  SMLoc StartLoc;
  SMLoc EndLoc;
  std::string TokenValue;
  MCRegister RegisterValue;
  MCRegister MemoryIndex;
  std::int64_t MemoryScale = 1;
  std::int64_t MemoryDisp = 0;
  const MCExpr *ImmediateValue = nullptr;

  explicit SeaBirdOperand(KindTy Kind) : Kind(Kind) {}

public:
  bool isToken() const override { return Kind == Token; }
  bool isReg() const override { return Kind == Register; }
  bool isImm() const override { return Kind == Immediate; }
  bool isMem() const override { return Kind == Memory; }
  bool isMemory() const { return isMem(); }
  StringRef getToken() const {
    assert(isToken());
    return TokenValue;
  }
  MCRegister getReg() const override {
    assert(isReg());
    return RegisterValue;
  }
  MCRegister getMemoryBase() const {
    assert(isMem());
    return RegisterValue;
  }
  MCRegister getMemoryIndex() const { assert(isMem()); return MemoryIndex; }
  std::int64_t getMemoryScale() const { assert(isMem()); return MemoryScale; }
  std::int64_t getMemoryDisp() const { assert(isMem()); return MemoryDisp; }
  const MCExpr *getImm() const {
    assert(isImm());
    return ImmediateValue;
  }
  SMLoc getStartLoc() const override { return StartLoc; }
  SMLoc getEndLoc() const override { return EndLoc; }

  void print(raw_ostream &OS, const MCAsmInfo &MAI) const override {
    if (isToken())
      OS << "Token: " << getToken();
    else if (isReg())
      OS << "Register: " << getReg();
    else if (isImm()) {
      OS << "Immediate: ";
      MAI.printExpr(OS, *getImm());
    } else
      OS << "Memory: [" << getMemoryBase() << ']';
  }

  void addRegOperands(MCInst &Inst, unsigned Count) const {
    assert(Count == 1);
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned Count) const {
    assert(Count == 1);
    if (const auto *Constant = dyn_cast<MCConstantExpr>(getImm()))
      Inst.addOperand(MCOperand::createImm(Constant->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(getImm()));
  }

  void addMemoryOperands(MCInst &Inst, unsigned Count) const {
    assert(Count == 4);
    Inst.addOperand(MCOperand::createReg(getMemoryBase()));
    Inst.addOperand(MCOperand::createReg(getMemoryIndex()));
    Inst.addOperand(MCOperand::createImm(getMemoryScale()));
    Inst.addOperand(MCOperand::createImm(getMemoryDisp()));
  }

  static std::unique_ptr<SeaBirdOperand> createToken(StringRef Value,
                                                     SMLoc Loc) {
    auto Operand = std::unique_ptr<SeaBirdOperand>(new SeaBirdOperand(Token));
    Operand->TokenValue = Value;
    Operand->StartLoc = Operand->EndLoc = Loc;
    return Operand;
  }

  static std::unique_ptr<SeaBirdOperand> createRegister(MCRegister Reg,
                                                        SMLoc Start,
                                                        SMLoc End) {
    auto Operand =
        std::unique_ptr<SeaBirdOperand>(new SeaBirdOperand(Register));
    Operand->RegisterValue = Reg;
    Operand->StartLoc = Start;
    Operand->EndLoc = End;
    return Operand;
  }

  static std::unique_ptr<SeaBirdOperand> createImmediate(const MCExpr *Expr,
                                                         SMLoc Start,
                                                         SMLoc End) {
    auto Operand =
        std::unique_ptr<SeaBirdOperand>(new SeaBirdOperand(Immediate));
    Operand->ImmediateValue = Expr;
    Operand->StartLoc = Start;
    Operand->EndLoc = End;
    return Operand;
  }

  static std::unique_ptr<SeaBirdOperand> createMemory(MCRegister Base,
                                                      MCRegister Index,
                                                      std::int64_t Scale,
                                                      std::int64_t Disp,
                                                      SMLoc Start,
                                                      SMLoc End) {
    auto Operand = std::unique_ptr<SeaBirdOperand>(new SeaBirdOperand(Memory));
    Operand->RegisterValue = Base;
    Operand->MemoryIndex = Index;
    Operand->MemoryScale = Scale;
    Operand->MemoryDisp = Disp;
    Operand->StartLoc = Start;
    Operand->EndLoc = End;
    return Operand;
  }
};

class SeaBirdAsmParser : public MCTargetAsmParser {
  MCAsmParser &Parser;
  const MCInstrInfo &MII;
  bool Is64Bit;
  unsigned PendingMarker = SeaBirdII::NoMarker;

  static unsigned parseMarkerName(StringRef Name) {
    return StringSwitch<unsigned>(Name.lower())
        .Case("assume", SeaBirdII::Assume)
        .Case("likely", SeaBirdII::Likely)
        .Case("unlikely", SeaBirdII::Unlikely)
        .Case("stream", SeaBirdII::Stream)
        .Case("prefetch", SeaBirdII::Prefetch)
        .Case("temporary", SeaBirdII::Temporary)
        .Case("persistent", SeaBirdII::Persistent)
        .Case("independent", SeaBirdII::Independent)
        .Case("reuse", SeaBirdII::Reuse)
        .Case("leaf", SeaBirdII::Leaf)
        .Default(SeaBirdII::NoMarker);
  }

#define GET_ASSEMBLER_HEADER
#include "SeaBirdGenAsmMatcher.inc"

  ParseStatus parseMemoryOperand(OperandVector &Operands) {
    if (Parser.getTok().isNot(AsmToken::LBrac))
      return ParseStatus::NoMatch;
    SMLoc Start = Parser.getTok().getLoc();
    Parser.Lex();
    auto Base = parseRegisterOperand();
    if (!Base)
      return ParseStatus::Failure;
    MCRegister Index = SeaBird::R4;
    std::int64_t Scale = 1;
    std::int64_t Disp = 0;
    while (Parser.getTok().isNot(AsmToken::RBrac)) {
      bool Negative = Parser.getTok().is(AsmToken::Minus);
      if (!Negative && Parser.getTok().isNot(AsmToken::Plus))
        return ParseStatus::Failure;
      Parser.Lex();
      if (Parser.getTok().is(AsmToken::Identifier)) {
        if (Negative || Index != SeaBird::R4)
          return ParseStatus::Failure;
        auto ParsedIndex = parseRegisterOperand();
        if (!ParsedIndex)
          return ParseStatus::Failure;
        Index = ParsedIndex->getReg();
        if (Parser.getTok().is(AsmToken::Star)) {
          Parser.Lex();
          if (Parser.getTok().isNot(AsmToken::Integer))
            return ParseStatus::Failure;
          Scale = Parser.getTok().getIntVal();
          Parser.Lex();
          if (Scale != 1 && Scale != 2 && Scale != 4 && Scale != 8)
            return ParseStatus::Failure;
        }
      } else {
        std::int64_t Value = 0;
        if (Parser.parseAbsoluteExpression(Value))
          return ParseStatus::Failure;
        Disp += Negative ? -Value : Value;
      }
    }
    MCRegister Reg = Base->getReg();
    SMLoc End = Parser.getTok().getEndLoc();
    Parser.Lex();
    Operands.push_back(SeaBirdOperand::createMemory(Reg, Index, Scale, Disp,
                                                    Start, End));
    return ParseStatus::Success;
  }

  std::unique_ptr<SeaBirdOperand> parseRegisterOperand() {
    if (Parser.getTok().isNot(AsmToken::Identifier))
      return nullptr;
    SMLoc Start = Parser.getTok().getLoc();
    SMLoc End = Parser.getTok().getEndLoc();
    MCRegister Reg = MatchRegisterName(Parser.getTok().getIdentifier());
    if (!Reg)
      return nullptr;
    Parser.Lex();
    return SeaBirdOperand::createRegister(Reg, Start, End);
  }

  ParseStatus parseOperand(OperandVector &Operands) {
    if (Parser.getTok().is(AsmToken::LBrac))
      return parseMemoryOperand(Operands);
    if (auto Reg = parseRegisterOperand()) {
      Operands.push_back(std::move(Reg));
      return ParseStatus::Success;
    }

    SMLoc Start = Parser.getTok().getLoc();
    const MCExpr *Expr = nullptr;
    if (Parser.parseExpression(Expr))
      return ParseStatus::Failure;
    Operands.push_back(
        SeaBirdOperand::createImmediate(Expr, Start, Parser.getTok().getLoc()));
    return ParseStatus::Success;
  }

  bool parseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override {
    PendingMarker = SeaBirdII::NoMarker;
    StringRef Mnemonic = Name;
    const size_t Dot = Name.find('.');
    if (Dot != StringRef::npos) {
      const unsigned ParsedMarker = parseMarkerName(Name.take_front(Dot));
      if (ParsedMarker != SeaBirdII::NoMarker) {
        PendingMarker = ParsedMarker;
        Mnemonic = Name.drop_front(Dot + 1);
        if (Mnemonic.empty())
          return Error(NameLoc, "performance marker requires an instruction");
      }
    }
    Operands.push_back(SeaBirdOperand::createToken(Mnemonic.lower(), NameLoc));
    if (Parser.getTok().is(AsmToken::EndOfStatement))
      return false;

    if (!parseOperand(Operands).isSuccess())
      return true;
    while (Parser.getTok().is(AsmToken::Comma)) {
      Parser.Lex();
      if (!parseOperand(Operands).isSuccess())
        return true;
    }
    return Parser.getTok().isNot(AsmToken::EndOfStatement);
  }

  bool parseRegister(MCRegister &Reg, SMLoc &StartLoc,
                     SMLoc &EndLoc) override {
    StartLoc = Parser.getTok().getLoc();
    EndLoc = Parser.getTok().getEndLoc();
    auto Operand = parseRegisterOperand();
    if (!Operand)
      return true;
    Reg = Operand->getReg();
    return false;
  }

  ParseStatus tryParseRegister(MCRegister &Reg, SMLoc &StartLoc,
                               SMLoc &EndLoc) override {
    StartLoc = Parser.getTok().getLoc();
    EndLoc = Parser.getTok().getEndLoc();
    auto Operand = parseRegisterOperand();
    if (!Operand)
      return ParseStatus::NoMatch;
    Reg = Operand->getReg();
    return ParseStatus::Success;
  }

  bool matchAndEmitInstruction(SMLoc IdLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override {
    MCInst Inst;
    switch (MatchInstructionImpl(Operands, Inst, ErrorInfo,
                                 MatchingInlineAsm)) {
    case Match_Success:
      if (PendingMarker != SeaBirdII::NoMarker) {
        const MCInstrDesc &Desc = MII.get(Inst.getOpcode());
        bool Valid = true;
        switch (PendingMarker) {
        case SeaBirdII::Assume:
          Valid = Desc.mayLoad() || Desc.mayStore();
          break;
        case SeaBirdII::Likely:
        case SeaBirdII::Unlikely:
          Valid = Desc.isConditionalBranch();
          break;
        case SeaBirdII::Stream:
          Valid = Desc.mayLoad() || Desc.mayStore();
          break;
        case SeaBirdII::Prefetch:
          Valid = Desc.mayLoad();
          break;
        case SeaBirdII::Temporary:
        case SeaBirdII::Persistent:
          Valid = Desc.getNumDefs() != 0;
          break;
        case SeaBirdII::Independent:
          Valid = !Desc.isBranch() && !Desc.isCall() && !Desc.isReturn() &&
                  !Desc.isBarrier();
          break;
        case SeaBirdII::Reuse:
        case SeaBirdII::Leaf:
          Valid = Desc.isCall();
          break;
        default:
          Valid = false;
        }
        if (!Valid)
          return Error(IdLoc,
                       "performance marker is not applicable to this instruction");
      }
      Inst.setFlags(PendingMarker);
      Out.emitInstruction(Inst, getSTI());
      Opcode = Inst.getOpcode();
      return false;
    case Match_MissingFeature:
      return Error(IdLoc, "instruction requires an unavailable feature");
    case Match_MnemonicFail:
      return Error(IdLoc, "unrecognized SeaBird instruction");
    case Match_InvalidOperand:
      return Error(ErrorInfo < Operands.size()
                       ? Operands[ErrorInfo]->getStartLoc()
                       : IdLoc,
                   "invalid SeaBird operand");
    default:
      return Error(IdLoc, "unable to match SeaBird instruction");
    }
  }

  ParseStatus parseCPUOrArchDirective(AsmToken DirectiveID) {
    const StringRef Value =
        Parser.parseStringToEndOfStatement().trim();
    if (Parser.parseEOL())
      return ParseStatus::Failure;

    StringRef CPU;
    if (DirectiveID.getString() == ".cpu") {
      if (Value == "generic" || Value == "seabird-gold" ||
          Value == "seabird-platinum" || Value == "tritium-v1" ||
          Value == "axium-m-v1")
        CPU = Value;
      else
        return Error(DirectiveID.getLoc(),
                     "unknown SeaBird CPU; expected generic, seabird-gold, "
                     "seabird-platinum, tritium-v1, or axium-m-v1");
    } else {
      if (Value == "seabird" || Value == "seabird64")
        CPU = "generic";
      else if (Value == "tritium" || Value == "tritium-v1")
        CPU = "tritium-v1";
      else if (Value == "axium-m" || Value == "axium-m-v1")
        CPU = "axium-m-v1";
      else
        return Error(DirectiveID.getLoc(),
                     "unknown SeaBird architecture; expected seabird, "
                     "seabird64, tritium, tritium-v1, axium-m, or axium-m-v1");
    }

    if (((CPU == "tritium-v1") || (CPU == "axium-m-v1")) == Is64Bit)
      return Error(DirectiveID.getLoc(),
                   CPU == "tritium-v1"
                       ? "tritium-v1 requires a seabird32 object"
                       : CPU == "axium-m-v1"
                       ? "axium-m-v1 requires a seabird32 object"
                       : "the general-purpose SeaBird profile requires a "
                         "seabird64 object");

    MCSubtargetInfo &STI = copySTI();
    STI.setDefaultFeatures(CPU, CPU, "");
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
    return ParseStatus::Success;
  }

  ParseStatus parseModeDirective(AsmToken DirectiveID) {
    const StringRef Mode = Parser.parseStringToEndOfStatement().trim();
    if (Parser.parseEOL())
      return ParseStatus::Failure;
    const bool Wants64 = Mode.equals_insensitive("dragonet");
    const bool Wants32 = Mode.equals_insensitive("tetra");
    if (!Wants64 && !Wants32)
      return Error(DirectiveID.getLoc(),
                   "unsupported SeaBird object mode; expected TETRA or "
                   "DRAGONET");
    if (Wants64 != Is64Bit)
      return Error(DirectiveID.getLoc(),
                   "SeaBird .mode cannot change object address width; select "
                   "a matching seabird32 or seabird64 triple");
    return ParseStatus::Success;
  }

  ParseStatus parseDirective(AsmToken DirectiveID) override {
    if (DirectiveID.getString() == ".cpu" ||
        DirectiveID.getString() == ".arch")
      return parseCPUOrArchDirective(DirectiveID);
    if (DirectiveID.getString() == ".mode")
      return parseModeDirective(DirectiveID);
    if (DirectiveID.getString() != ".message")
      return ParseStatus::NoMatch;
    if (Parser.getTok().isNot(AsmToken::String))
      return Error(Parser.getTok().getLoc(),
                   "expected a string after .message");
    const std::string Message = Parser.getTok().getStringContents().str();
    Parser.Lex();
    if (Parser.parseEOL())
      return ParseStatus::Failure;
    Parser.getSourceManager().PrintMessage(
        DirectiveID.getLoc(), SourceMgr::DK_Note, Message);
    return ParseStatus::Success;
  }

public:
  SeaBirdAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser,
                   const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII), Parser(Parser), MII(MII),
        Is64Bit(STI.getTargetTriple().isArch64Bit()) {
    Parser.addAliasForDirective(".word", ".2byte");
    Parser.addAliasForDirective(".dword", ".4byte");
    Parser.addAliasForDirective(".qword", ".8byte");
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }
};

} // namespace

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "SeaBirdGenAsmMatcher.inc"

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeSeaBirdAsmParser() {
  RegisterMCAsmParser<SeaBirdAsmParser> Parser32(getTheSeaBird32Target());
  RegisterMCAsmParser<SeaBirdAsmParser> Parser64(getTheSeaBird64Target());
}
