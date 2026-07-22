#include "MCTargetDesc/SeaBirdMCTargetDesc.h"
#include "TargetInfo/SeaBirdTargetInfo.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

#include <cstdint>

using namespace llvm;

namespace {

enum class DecodeForm {
  CopyRR,
  ALURR,
  CompareRR,
  RI64,
  LoadQ,
  StoreQ,
  IndirectCall,
  FPBinary,
  FPXFused,
  DSPTernary,
  DSPQuaternary,
  DSPDot,
  DSPSumDot,
  AVXRegImm,
  AVXBinaryImm,
  AVXGather,
  AVXScatter,
  VBinary,
  CopyVV,
  ScalarFPLoad,
  ScalarFPStore,
  AddressCalc,
  VectorLoad,
  VectorStore,
  VectorAtomic,
  IntToFP,
  FPToInt,
  UnaryInPlace,
  UnaryRR,
  ALURI,
  CompareRI,
  TrapImm,
  RegCondBranch,
  StackReg,
  FrameImm,
  TernaryRI,
  TernaryByte,
  ReadReg,
  ControlReg,
  SysXWriteImm,
  SysXMemory,
  SysXMaskedMemory,
  SysXStoreMemory,
  AtomicMem,
  AtomicLoad,
  TernaryRR,
  MemoryOnly,
  PairLoad,
  PairStore,
  AtomicCAS,
  Branch,
  NoOperand
};

class SeaBirdDisassembler final : public MCDisassembler {
  bool Is64Bit;

  static MCRegister decodeRegister(unsigned Number) {
    return MCRegister(SeaBird::R0 + Number);
  }
  static MCRegister decodeVector(unsigned Number) {
    return MCRegister(SeaBird::V0 + Number);
  }

  static bool decodeOpcode(std::uint8_t Byte, unsigned &Opcode,
                           DecodeForm &Form) {
    switch (Byte) {
#define SEABIRD_FORM_CopyRR DecodeForm::CopyRR
#define SEABIRD_FORM_ALURR DecodeForm::ALURR
#define SEABIRD_FORM_CompareRR DecodeForm::CompareRR
#define SEABIRD_FORM_RI64 DecodeForm::RI64
#define SEABIRD_FORM_LoadQ DecodeForm::LoadQ
#define SEABIRD_FORM_StoreQ DecodeForm::StoreQ
#define SEABIRD_FORM_IndirectCall DecodeForm::IndirectCall
#define SEABIRD_FORM_FPBinary DecodeForm::FPBinary
#define SEABIRD_FORM_FPUnary DecodeForm::CopyVV
#define SEABIRD_FORM_FPCompare DecodeForm::CopyVV
#define SEABIRD_FORM_VBinary DecodeForm::VBinary
#define SEABIRD_FORM_VUnary DecodeForm::CopyVV
#define SEABIRD_FORM_VShiftImm DecodeForm::TernaryByte
#define SEABIRD_FORM_AddressCalc DecodeForm::AddressCalc
#define SEABIRD_FORM_VectorLoad DecodeForm::VectorLoad
#define SEABIRD_FORM_VectorStore DecodeForm::VectorStore
#define SEABIRD_FORM_IntToFP DecodeForm::IntToFP
#define SEABIRD_FORM_FPToInt DecodeForm::FPToInt
#define SEABIRD_FORM_UnaryInPlace DecodeForm::UnaryInPlace
#define SEABIRD_FORM_UnaryRR DecodeForm::UnaryRR
#define SEABIRD_FORM_ALURI DecodeForm::ALURI
#define SEABIRD_FORM_CompareRI DecodeForm::CompareRI
#define SEABIRD_FORM_TrapImm DecodeForm::TrapImm
#define SEABIRD_FORM_RegCondBranch DecodeForm::RegCondBranch
#define SEABIRD_FORM_PushReg DecodeForm::StackReg
#define SEABIRD_FORM_PopReg DecodeForm::StackReg
#define SEABIRD_FORM_FrameImm DecodeForm::FrameImm
#define SEABIRD_FORM_TernaryRI DecodeForm::TernaryRI
#define SEABIRD_FORM_TernaryByte DecodeForm::TernaryByte
#define SEABIRD_FORM_ReadReg DecodeForm::ReadReg
#define SEABIRD_FORM_ReadControl DecodeForm::ControlReg
#define SEABIRD_FORM_WriteControl DecodeForm::ControlReg
#define SEABIRD_FORM_SleepImm DecodeForm::FrameImm
#define SEABIRD_FORM_SystemNoOperand DecodeForm::NoOperand
#define SEABIRD_FORM_SysXQuery DecodeForm::CompareRR
#define SEABIRD_FORM_SysXReadImm DecodeForm::ControlReg
#define SEABIRD_FORM_SysXWriteImm DecodeForm::SysXWriteImm
#define SEABIRD_FORM_SysXMaskedMemory DecodeForm::SysXMaskedMemory
#define SEABIRD_FORM_SysXNoOperand DecodeForm::NoOperand
#define SEABIRD_FORM_SysXPair DecodeForm::CompareRR
#define SEABIRD_FORM_SysXTernary DecodeForm::TernaryRR
#define SEABIRD_FORM_SysXReg DecodeForm::StackReg
#define SEABIRD_FORM_SysXReadReg DecodeForm::ReadReg
#define SEABIRD_FORM_SysXMemory DecodeForm::SysXMemory
#define SEABIRD_FORM_SysXStoreMemory DecodeForm::SysXStoreMemory
#define SEABIRD_FORM_SysXImm DecodeForm::TrapImm
#define SEABIRD_FORM_AtomicMem DecodeForm::AtomicMem
#define SEABIRD_FORM_AtomicLoad DecodeForm::AtomicLoad
#define SEABIRD_FORM_TernaryRR DecodeForm::TernaryRR
#define SEABIRD_FORM_MemoryOnly DecodeForm::MemoryOnly
#define SEABIRD_FORM_PairLoad DecodeForm::PairLoad
#define SEABIRD_FORM_PairStore DecodeForm::PairStore
#define SEABIRD_FORM_AtomicCAS DecodeForm::AtomicCAS
#define SEABIRD_FORM_AtomicExchange DecodeForm::AtomicMem
#define SEABIRD_FORM_AtomicExchange128 DecodeForm::VectorAtomic
#define SEABIRD_FORM_TxnBeginRel DecodeForm::Branch
#define SEABIRD_FORM_TxnBeginAbs DecodeForm::IndirectCall
#define SEABIRD_FORM_TxnNoOperand DecodeForm::NoOperand
#define SEABIRD_FORM_TxnAbortImm DecodeForm::TrapImm
#define SEABIRD_FORM_TxnAbortReg DecodeForm::StackReg
#define SEABIRD_FORM_TxnReadReg DecodeForm::ReadReg
#define SEABIRD_FORM_IndirectBranch DecodeForm::IndirectCall
#define SEABIRD_FORM_UncondBranch DecodeForm::Branch
#define SEABIRD_FORM_CondBranch DecodeForm::Branch
#define SEABIRD_FORM_Call DecodeForm::Branch
#define SEABIRD_FORM_NoOperand DecodeForm::NoOperand
#define SEABIRD_FORM_Return DecodeForm::NoOperand
#define SEABIRD_OPCODE(Name, ByteValue, RecordClass)                         \
  case ByteValue:                                                           \
    Opcode = SeaBird::Name;                                                 \
    Form = SEABIRD_FORM_##RecordClass;                                      \
    return true;
#define SEABIRD_SYSX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_FPX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_CRYPTO_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_DSP_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_AVX_OPCODE(Name, ByteValue, RecordClass)
#include "SeaBirdGenOpcodeMap.inc"
#undef SEABIRD_AVX_OPCODE
#undef SEABIRD_DSP_OPCODE
#undef SEABIRD_CRYPTO_OPCODE
#undef SEABIRD_FPX_OPCODE
#undef SEABIRD_SYSX_OPCODE
#undef SEABIRD_OPCODE
#undef SEABIRD_FORM_CopyRR
#undef SEABIRD_FORM_ALURR
#undef SEABIRD_FORM_CompareRR
#undef SEABIRD_FORM_RI64
#undef SEABIRD_FORM_LoadQ
#undef SEABIRD_FORM_StoreQ
#undef SEABIRD_FORM_IndirectCall
#undef SEABIRD_FORM_FPBinary
#undef SEABIRD_FORM_FPUnary
#undef SEABIRD_FORM_FPCompare
#undef SEABIRD_FORM_VBinary
#undef SEABIRD_FORM_VUnary
#undef SEABIRD_FORM_VShiftImm
#undef SEABIRD_FORM_AddressCalc
#undef SEABIRD_FORM_VectorLoad
#undef SEABIRD_FORM_VectorStore
#undef SEABIRD_FORM_IntToFP
#undef SEABIRD_FORM_FPToInt
#undef SEABIRD_FORM_UnaryInPlace
#undef SEABIRD_FORM_UnaryRR
#undef SEABIRD_FORM_ALURI
#undef SEABIRD_FORM_CompareRI
#undef SEABIRD_FORM_TrapImm
#undef SEABIRD_FORM_RegCondBranch
#undef SEABIRD_FORM_PushReg
#undef SEABIRD_FORM_PopReg
#undef SEABIRD_FORM_FrameImm
#undef SEABIRD_FORM_TernaryRI
#undef SEABIRD_FORM_TernaryByte
#undef SEABIRD_FORM_ReadReg
#undef SEABIRD_FORM_ReadControl
#undef SEABIRD_FORM_WriteControl
#undef SEABIRD_FORM_SleepImm
#undef SEABIRD_FORM_SystemNoOperand
#undef SEABIRD_FORM_SysXQuery
#undef SEABIRD_FORM_SysXReadImm
#undef SEABIRD_FORM_SysXWriteImm
#undef SEABIRD_FORM_SysXMaskedMemory
#undef SEABIRD_FORM_SysXNoOperand
#undef SEABIRD_FORM_SysXPair
#undef SEABIRD_FORM_SysXTernary
#undef SEABIRD_FORM_SysXReg
#undef SEABIRD_FORM_SysXReadReg
#undef SEABIRD_FORM_SysXMemory
#undef SEABIRD_FORM_SysXStoreMemory
#undef SEABIRD_FORM_SysXImm
#undef SEABIRD_FORM_AtomicMem
#undef SEABIRD_FORM_AtomicLoad
#undef SEABIRD_FORM_TernaryRR
#undef SEABIRD_FORM_MemoryOnly
#undef SEABIRD_FORM_PairLoad
#undef SEABIRD_FORM_PairStore
#undef SEABIRD_FORM_AtomicCAS
#undef SEABIRD_FORM_AtomicExchange
#undef SEABIRD_FORM_AtomicExchange128
#undef SEABIRD_FORM_TxnBeginRel
#undef SEABIRD_FORM_TxnBeginAbs
#undef SEABIRD_FORM_TxnNoOperand
#undef SEABIRD_FORM_TxnAbortImm
#undef SEABIRD_FORM_TxnAbortReg
#undef SEABIRD_FORM_TxnReadReg
#undef SEABIRD_FORM_IndirectBranch
#undef SEABIRD_FORM_UncondBranch
#undef SEABIRD_FORM_CondBranch
#undef SEABIRD_FORM_Call
#undef SEABIRD_FORM_NoOperand
#undef SEABIRD_FORM_Return
    default:
      return false;
    }
  }

  static bool decodeSysXOpcode(std::uint8_t Byte, unsigned &Opcode,
                               DecodeForm &Form) {
    switch (Byte) {
#define SEABIRD_FORM_SysXQuery DecodeForm::CompareRR
#define SEABIRD_FORM_SysXReadImm DecodeForm::ControlReg
#define SEABIRD_FORM_SysXWriteImm DecodeForm::SysXWriteImm
#define SEABIRD_FORM_SysXMaskedMemory DecodeForm::SysXMaskedMemory
#define SEABIRD_FORM_SysXNoOperand DecodeForm::NoOperand
#define SEABIRD_FORM_SysXPair DecodeForm::CompareRR
#define SEABIRD_FORM_SysXTernary DecodeForm::TernaryRR
#define SEABIRD_FORM_SysXReg DecodeForm::StackReg
#define SEABIRD_FORM_SysXReadReg DecodeForm::ReadReg
#define SEABIRD_FORM_SysXMemory DecodeForm::SysXMemory
#define SEABIRD_FORM_SysXStoreMemory DecodeForm::SysXStoreMemory
#define SEABIRD_FORM_SysXImm DecodeForm::TrapImm
#define SEABIRD_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_FPX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_CRYPTO_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_DSP_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_AVX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_SYSX_OPCODE(Name, ByteValue, RecordClass)                    \
  case ByteValue:                                                           \
    Opcode = SeaBird::Name;                                                 \
    Form = SEABIRD_FORM_##RecordClass;                                      \
    return true;
#include "SeaBirdGenOpcodeMap.inc"
#undef SEABIRD_AVX_OPCODE
#undef SEABIRD_DSP_OPCODE
#undef SEABIRD_CRYPTO_OPCODE
#undef SEABIRD_SYSX_OPCODE
#undef SEABIRD_FPX_OPCODE
#undef SEABIRD_OPCODE
#undef SEABIRD_FORM_SysXQuery
#undef SEABIRD_FORM_SysXReadImm
#undef SEABIRD_FORM_SysXWriteImm
#undef SEABIRD_FORM_SysXMaskedMemory
#undef SEABIRD_FORM_SysXNoOperand
#undef SEABIRD_FORM_SysXPair
#undef SEABIRD_FORM_SysXTernary
#undef SEABIRD_FORM_SysXReg
#undef SEABIRD_FORM_SysXReadReg
#undef SEABIRD_FORM_SysXMemory
#undef SEABIRD_FORM_SysXStoreMemory
#undef SEABIRD_FORM_SysXImm
    default:
      return false;
    }
  }

  static bool decodeFPXOpcode(std::uint8_t Byte, unsigned &Opcode,
                              DecodeForm &Form) {
    switch (Byte) {
#define SEABIRD_FORM_FPXFused DecodeForm::FPXFused
#define SEABIRD_FORM_FPXBinary DecodeForm::FPBinary
#define SEABIRD_FORM_FPXUnary DecodeForm::CopyVV
#define SEABIRD_FORM_FPXLoad DecodeForm::ScalarFPLoad
#define SEABIRD_FORM_FPXStore DecodeForm::ScalarFPStore
#define SEABIRD_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_SYSX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_CRYPTO_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_DSP_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_AVX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_FPX_OPCODE(Name, ByteValue, RecordClass)                     \
  case ByteValue:                                                           \
    Opcode = SeaBird::Name;                                                 \
    Form = SEABIRD_FORM_##RecordClass;                                      \
    return true;
#include "SeaBirdGenOpcodeMap.inc"
#undef SEABIRD_AVX_OPCODE
#undef SEABIRD_DSP_OPCODE
#undef SEABIRD_FPX_OPCODE
#undef SEABIRD_CRYPTO_OPCODE
#undef SEABIRD_SYSX_OPCODE
#undef SEABIRD_OPCODE
#undef SEABIRD_FORM_FPXFused
#undef SEABIRD_FORM_FPXBinary
#undef SEABIRD_FORM_FPXUnary
#undef SEABIRD_FORM_FPXLoad
#undef SEABIRD_FORM_FPXStore
    default:
      return false;
    }
  }

  static bool decodeCryptoOpcode(std::uint8_t Byte, unsigned &Opcode,
                                 DecodeForm &Form) {
    switch (Byte) {
#define SEABIRD_FORM_CryptoBinary DecodeForm::VBinary
#define SEABIRD_FORM_CryptoUnary DecodeForm::CopyVV
#define SEABIRD_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_SYSX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_FPX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_DSP_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_AVX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_CRYPTO_OPCODE(Name, ByteValue, RecordClass)                  \
  case ByteValue:                                                           \
    Opcode = SeaBird::Name;                                                 \
    Form = SEABIRD_FORM_##RecordClass;                                      \
    return true;
#include "SeaBirdGenOpcodeMap.inc"
#undef SEABIRD_AVX_OPCODE
#undef SEABIRD_DSP_OPCODE
#undef SEABIRD_CRYPTO_OPCODE
#undef SEABIRD_FPX_OPCODE
#undef SEABIRD_SYSX_OPCODE
#undef SEABIRD_OPCODE
#undef SEABIRD_FORM_CryptoBinary
#undef SEABIRD_FORM_CryptoUnary
    default:
      return false;
    }
  }

  static bool decodeDSPOpcode(std::uint8_t Byte, unsigned &Opcode,
                              DecodeForm &Form) {
    switch (Byte) {
#define SEABIRD_FORM_DSPTernary DecodeForm::DSPTernary
#define SEABIRD_FORM_DSPQuaternary DecodeForm::DSPQuaternary
#define SEABIRD_FORM_DSPPairOut DecodeForm::DSPQuaternary
#define SEABIRD_FORM_DSPUnary DecodeForm::CompareRR
#define SEABIRD_FORM_DSPRegImm DecodeForm::TernaryByte
#define SEABIRD_FORM_DSPDot DecodeForm::DSPDot
#define SEABIRD_FORM_DSPSumDot DecodeForm::DSPSumDot
#define SEABIRD_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_SYSX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_FPX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_CRYPTO_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_AVX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_DSP_OPCODE(Name, ByteValue, RecordClass)                     \
  case ByteValue:                                                           \
    Opcode = SeaBird::Name;                                                 \
    Form = SEABIRD_FORM_##RecordClass;                                      \
    return true;
#include "SeaBirdGenOpcodeMap.inc"
#undef SEABIRD_AVX_OPCODE
#undef SEABIRD_DSP_OPCODE
#undef SEABIRD_CRYPTO_OPCODE
#undef SEABIRD_FPX_OPCODE
#undef SEABIRD_SYSX_OPCODE
#undef SEABIRD_OPCODE
#undef SEABIRD_FORM_DSPTernary
#undef SEABIRD_FORM_DSPQuaternary
#undef SEABIRD_FORM_DSPPairOut
#undef SEABIRD_FORM_DSPUnary
#undef SEABIRD_FORM_DSPRegImm
#undef SEABIRD_FORM_DSPDot
#undef SEABIRD_FORM_DSPSumDot
    default:
      return false;
    }
  }

  static bool decodeAVXOpcode(std::uint8_t Byte, unsigned &Opcode,
                              DecodeForm &Form) {
    switch (Byte) {
#define SEABIRD_FORM_AVXBinary DecodeForm::VBinary
#define SEABIRD_FORM_AVXUnary DecodeForm::CopyVV
#define SEABIRD_FORM_AVXRegImm DecodeForm::AVXRegImm
#define SEABIRD_FORM_AVXBinaryImm DecodeForm::AVXBinaryImm
#define SEABIRD_FORM_AVXGather DecodeForm::AVXGather
#define SEABIRD_FORM_AVXScatter DecodeForm::AVXScatter
#define SEABIRD_FORM_AVXNoOperand DecodeForm::NoOperand
#define SEABIRD_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_SYSX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_FPX_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_CRYPTO_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_DSP_OPCODE(Name, ByteValue, RecordClass)
#define SEABIRD_AVX_OPCODE(Name, ByteValue, RecordClass)                     \
  case ByteValue:                                                           \
    Opcode = SeaBird::Name;                                                 \
    Form = SEABIRD_FORM_##RecordClass;                                      \
    return true;
#include "SeaBirdGenOpcodeMap.inc"
#undef SEABIRD_AVX_OPCODE
#undef SEABIRD_DSP_OPCODE
#undef SEABIRD_CRYPTO_OPCODE
#undef SEABIRD_FPX_OPCODE
#undef SEABIRD_SYSX_OPCODE
#undef SEABIRD_OPCODE
#undef SEABIRD_FORM_AVXBinary
#undef SEABIRD_FORM_AVXUnary
#undef SEABIRD_FORM_AVXRegImm
#undef SEABIRD_FORM_AVXBinaryImm
#undef SEABIRD_FORM_AVXGather
#undef SEABIRD_FORM_AVXScatter
#undef SEABIRD_FORM_AVXNoOperand
    default:
      return false;
    }
  }

public:
  SeaBirdDisassembler(const MCSubtargetInfo &STI, MCContext &Context)
      : MCDisassembler(STI, Context),
        Is64Bit(STI.getTargetTriple().isArch64Bit()) {}

  DecodeStatus getInstruction(MCInst &Inst, std::uint64_t &Size,
                              ArrayRef<std::uint8_t> Bytes,
                              std::uint64_t Address,
                              raw_ostream &CStream) const override {
    Size = 0;
    if (Bytes.empty())
      return Fail;

    bool Extended = false;
    bool HasVectorCtl = false;
    unsigned OperandWidth = 0;
    unsigned OpcodeOffset = 0;
    if (Bytes[0] == 0xFE) {
      if (Bytes.size() < 3 || (Bytes[1] & 0x07) != 0)
        return Fail;
      Extended = (Bytes[1] & 0x80) != 0;
      HasVectorCtl = (Bytes[1] & 0x40) != 0;
      OperandWidth = (Bytes[1] >> 3) & 7;
      OpcodeOffset = 2;
    }

    unsigned Opcode = 0;
    DecodeForm Form = DecodeForm::NoOperand;
    unsigned OpcodeBytes = 1;
    if (Bytes[OpcodeOffset] == 0xFF &&
        Bytes.size() >= OpcodeOffset + 3 &&
        Bytes[OpcodeOffset + 1] == 0x01 &&
        decodeAVXOpcode(Bytes[OpcodeOffset + 2], Opcode, Form)) {
      OpcodeBytes = 3;
    } else if (Bytes[OpcodeOffset] == 0xFF &&
        Bytes.size() >= OpcodeOffset + 3 &&
        Bytes[OpcodeOffset + 1] == 0x02 &&
        decodeCryptoOpcode(Bytes[OpcodeOffset + 2], Opcode, Form)) {
      OpcodeBytes = 3;
    } else if (Bytes[OpcodeOffset] == 0xFF &&
               Bytes.size() >= OpcodeOffset + 3 &&
               Bytes[OpcodeOffset + 1] == 0x03 &&
               decodeDSPOpcode(Bytes[OpcodeOffset + 2], Opcode, Form)) {
      OpcodeBytes = 3;
    } else if (Bytes[OpcodeOffset] == 0xFF &&
        Bytes.size() >= OpcodeOffset + 3 &&
        Bytes[OpcodeOffset + 1] == 0x05 &&
        decodeFPXOpcode(Bytes[OpcodeOffset + 2], Opcode, Form)) {
      OpcodeBytes = 3;
    } else if (Bytes[OpcodeOffset] == 0xFF &&
               Bytes.size() >= OpcodeOffset + 3 &&
               Bytes[OpcodeOffset + 1] == 0x04 &&
               decodeSysXOpcode(Bytes[OpcodeOffset + 2], Opcode, Form)) {
      OpcodeBytes = 3;
    } else if (!decodeOpcode(Bytes[OpcodeOffset], Opcode, Form)) {
      return Fail;
    }
    Inst.setOpcode(Opcode);
    if (HasVectorCtl && Opcode == SeaBird::MOVrr) {
      Inst.setOpcode(SeaBird::MOVV128);
      Form = DecodeForm::CopyVV;
    }

    if (Form == DecodeForm::NoOperand) {
      if (Extended || HasVectorCtl)
        return Fail;
      Size = OpcodeOffset + OpcodeBytes;
      return Success;
    }

    if (Form == DecodeForm::TrapImm) {
      if (Extended || HasVectorCtl ||
          Bytes.size() <= OpcodeOffset + OpcodeBytes)
        return Fail;
      Inst.addOperand(
          MCOperand::createImm(Bytes[OpcodeOffset + OpcodeBytes]));
      Size = OpcodeOffset + OpcodeBytes + 1;
      return Success;
    }

    if (Form == DecodeForm::FrameImm) {
      if (Extended || HasVectorCtl)
        return Fail;
      const unsigned ImmediateBytes = Is64Bit ? 8 : 4;
      if (Bytes.size() < OpcodeBytes + ImmediateBytes)
        return Fail;
      std::uint64_t Immediate = 0;
      for (unsigned I = 0; I < ImmediateBytes; ++I)
        Immediate |= std::uint64_t(Bytes[OpcodeBytes + I]) << (I * 8);
      Inst.addOperand(MCOperand::createImm(
          static_cast<std::int64_t>(Immediate)));
      Size = OpcodeBytes + ImmediateBytes;
      return Success;
    }

    if (Form == DecodeForm::Branch) {
      if (Extended || HasVectorCtl || Bytes.size() < 5)
        return Fail;
      const std::uint32_t Raw = static_cast<std::uint32_t>(Bytes[1]) |
                                (static_cast<std::uint32_t>(Bytes[2]) << 8) |
                                (static_cast<std::uint32_t>(Bytes[3]) << 16) |
                                (static_cast<std::uint32_t>(Bytes[4]) << 24);
      Inst.addOperand(MCOperand::createImm(static_cast<std::int32_t>(Raw)));
      Size = 5;
      return Success;
    }

    const unsigned ModRMOffset = OpcodeOffset + OpcodeBytes;
    if (Bytes.size() <= ModRMOffset)
      return Fail;
    const std::uint8_t ModRM = Bytes[ModRMOffset];
    const bool MemoryForm =
        Form == DecodeForm::LoadQ || Form == DecodeForm::StoreQ ||
        Form == DecodeForm::AddressCalc ||
        Form == DecodeForm::SysXMemory ||
        Form == DecodeForm::SysXMaskedMemory ||
        Form == DecodeForm::SysXStoreMemory ||
        Form == DecodeForm::AtomicMem ||
        Form == DecodeForm::AtomicLoad ||
        Form == DecodeForm::MemoryOnly ||
        Form == DecodeForm::PairStore ||
        Form == DecodeForm::AtomicCAS ||
        Form == DecodeForm::VectorAtomic ||
        Form == DecodeForm::AVXGather ||
        Form == DecodeForm::AVXScatter ||
        Form == DecodeForm::VectorLoad || Form == DecodeForm::VectorStore ||
        Form == DecodeForm::ScalarFPLoad || Form == DecodeForm::ScalarFPStore;
    const unsigned Mod = ModRM >> 6;
    if ((!MemoryForm && Mod != 3) ||
        (MemoryForm && Mod == 3))
      return Fail;
    unsigned Reg = (ModRM >> 3) & 7;
    unsigned RM = ModRM & 7;
    unsigned PayloadOffset = ModRMOffset + 1;
    bool HasSIB = MemoryForm && Mod != 3 && RM == 4;
    std::uint8_t SIB = 0;
    unsigned Index = 4;
    unsigned Scale = 1;
    if (HasSIB) {
      if (Bytes.size() <= PayloadOffset)
        return Fail;
      SIB = Bytes[PayloadOffset++];
      Index = (SIB >> 3) & 7;
      Scale = 1U << (SIB >> 6);
      RM = SIB & 7;
    }
    if (Extended) {
      if (Bytes.size() <= PayloadOffset)
        return Fail;
      const std::uint8_t OREX = Bytes[PayloadOffset++];
      if (!HasSIB && (OREX & 0xF0))
        return Fail;
      Reg |= (OREX & 3) << 3;
      if (HasSIB) {
        RM |= ((OREX >> 6) & 3) << 3;
        Index |= ((OREX >> 4) & 3) << 3;
      } else {
        RM |= ((OREX >> 2) & 3) << 3;
      }
    }

    unsigned ScalarLaneWidth = 0;
    if (HasVectorCtl) {
      if (Bytes.size() <= PayloadOffset)
        return Fail;
      const std::uint8_t VectorCtl = Bytes[PayloadOffset++];
      if ((VectorCtl & 0xC7) != 0)
        return Fail;
      ScalarLaneWidth = (VectorCtl >> 3) & 7;
    }

    if (OperandWidth == 3) {
      if (Opcode == SeaBird::FLD64)
        Inst.setOpcode(SeaBird::FLD32CG);
      else if (Opcode == SeaBird::FST64)
        Inst.setOpcode(SeaBird::FST32CG);
    }
    if (OperandWidth == 5) {
      if (Opcode == SeaBird::FLD64)
        Inst.setOpcode(SeaBird::FLD128CG);
      else if (Opcode == SeaBird::FST64)
        Inst.setOpcode(SeaBird::FST128CG);
    }
    if (HasVectorCtl && ScalarLaneWidth == 2) {
      switch (Opcode) {
      case SeaBird::FADD64: Inst.setOpcode(SeaBird::FADD32CG); break;
      case SeaBird::FSUB64: Inst.setOpcode(SeaBird::FSUB32CG); break;
      case SeaBird::FMUL64: Inst.setOpcode(SeaBird::FMUL32CG); break;
      case SeaBird::FDIV64: Inst.setOpcode(SeaBird::FDIV32CG); break;
      case SeaBird::FNEG64: Inst.setOpcode(SeaBird::FNEG32CG); break;
      case SeaBird::FABS64: Inst.setOpcode(SeaBird::FABS32CG); break;
      case SeaBird::FSQRT64: Inst.setOpcode(SeaBird::FSQRT32CG); break;
      case SeaBird::FCMP64: Inst.setOpcode(SeaBird::FCMP32CG); break;
      case SeaBird::FMADD64: Inst.setOpcode(SeaBird::FMADD32CG); break;
      case SeaBird::FMSUB64: Inst.setOpcode(SeaBird::FMSUB32CG); break;
      case SeaBird::FMIN64: Inst.setOpcode(SeaBird::FMIN32CG); break;
      case SeaBird::FMAX64: Inst.setOpcode(SeaBird::FMAX32CG); break;
      case SeaBird::FCVTI64: Inst.setOpcode(SeaBird::FCVTI32CG); break;
      case SeaBird::FCVTS64: Inst.setOpcode(SeaBird::FCVTS32CG); break;
      default: break;
      }
    }
    if (HasVectorCtl && ScalarLaneWidth == 4) {
      switch (Opcode) {
      case SeaBird::FADD64: Inst.setOpcode(SeaBird::FADD128CG); break;
      case SeaBird::FSUB64: Inst.setOpcode(SeaBird::FSUB128CG); break;
      case SeaBird::FMUL64: Inst.setOpcode(SeaBird::FMUL128CG); break;
      case SeaBird::FDIV64: Inst.setOpcode(SeaBird::FDIV128CG); break;
      case SeaBird::FNEG64: Inst.setOpcode(SeaBird::FNEG128CG); break;
      case SeaBird::FABS64: Inst.setOpcode(SeaBird::FABS128CG); break;
      case SeaBird::FSQRT64: Inst.setOpcode(SeaBird::FSQRT128CG); break;
      case SeaBird::FCMP64: Inst.setOpcode(SeaBird::FCMP128CG); break;
      case SeaBird::FMADD64: Inst.setOpcode(SeaBird::FMADD128CG); break;
      case SeaBird::FMSUB64: Inst.setOpcode(SeaBird::FMSUB128CG); break;
      case SeaBird::FMIN64: Inst.setOpcode(SeaBird::FMIN128CG); break;
      case SeaBird::FMAX64: Inst.setOpcode(SeaBird::FMAX128CG); break;
      case SeaBird::FCVTI64: Inst.setOpcode(SeaBird::FCVTI128CG); break;
      case SeaBird::FCVTS64: Inst.setOpcode(SeaBird::FCVTS128CG); break;
      default: break;
      }
    }

    unsigned AVXMask = 0;
    if (Form == DecodeForm::AVXGather ||
        Form == DecodeForm::AVXScatter) {
      if (!HasVectorCtl || Bytes.size() <= PayloadOffset)
        return Fail;
      AVXMask = Bytes[PayloadOffset++];
    }

    if (Form == DecodeForm::PairLoad) {
      if (HasVectorCtl || Bytes.size() <= PayloadOffset ||
          Bytes[PayloadOffset] > 31)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeRegister(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
      Inst.addOperand(
          MCOperand::createReg(decodeRegister(Bytes[PayloadOffset])));
      Inst.addOperand(MCOperand::createReg(SeaBird::R4));
      Inst.addOperand(MCOperand::createImm(1));
      Inst.addOperand(MCOperand::createImm(0));
      Size = PayloadOffset + 1;
      return Success;
    }

    unsigned PairStoreExtra = 0;
    if (Form == DecodeForm::PairStore) {
      if (Bytes.size() <= PayloadOffset || Bytes[PayloadOffset] > 31)
        return Fail;
      PairStoreExtra = Bytes[PayloadOffset++];
    }
    unsigned AtomicCASDesired = 0;
    if (Form == DecodeForm::AtomicCAS) {
      if (Bytes.size() <= PayloadOffset || Bytes[PayloadOffset] > 31)
        return Fail;
      AtomicCASDesired = Bytes[PayloadOffset++];
    }

    if (Form == DecodeForm::RI64 || Form == DecodeForm::ALURI ||
        Form == DecodeForm::CompareRI) {
      const unsigned ImmediateBytes = Is64Bit ? 8 : 4;
      if (Reg != 0 || Bytes.size() < PayloadOffset + ImmediateBytes)
        return Fail;
      std::uint64_t Immediate = 0;
      for (unsigned I = 0; I < ImmediateBytes; ++I)
        Immediate |= std::uint64_t(Bytes[PayloadOffset + I]) << (I * 8);
      const MCRegister Dst = decodeRegister(RM);
      Inst.addOperand(MCOperand::createReg(Dst));
      if (Form == DecodeForm::ALURI)
        Inst.addOperand(MCOperand::createReg(Dst));
      const std::int64_t PrintedImmediate =
          Form != DecodeForm::RI64 && !Is64Bit
              ? static_cast<std::int32_t>(Immediate)
              : static_cast<std::int64_t>(Immediate);
      Inst.addOperand(MCOperand::createImm(PrintedImmediate));
      Size = PayloadOffset + ImmediateBytes;
      return Success;
    }

    if (Form == DecodeForm::IndirectCall) {
      if (Reg != 0)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::RegCondBranch) {
      if (Reg != 0 || Bytes.size() < PayloadOffset + 4)
        return Fail;
      std::uint32_t Raw = 0;
      for (unsigned I = 0; I < 4; ++I)
        Raw |= std::uint32_t(Bytes[PayloadOffset + I]) << (I * 8);
      Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
      Inst.addOperand(
          MCOperand::createImm(static_cast<std::int32_t>(Raw)));
      Size = PayloadOffset + 4;
      return Success;
    }

    if (Form == DecodeForm::UnaryInPlace) {
      if (Reg != 0)
        return Fail;
      const MCRegister Dst = decodeRegister(RM);
      Inst.addOperand(MCOperand::createReg(Dst));
      Inst.addOperand(MCOperand::createReg(Dst));
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::StackReg ||
        Form == DecodeForm::ReadReg) {
      if (Reg != 0)
        return Fail;
      if ((Inst.getOpcode() == SeaBird::PUSHQ ||
           Inst.getOpcode() == SeaBird::POPQ) &&
          (RM & 1 || RM == 31))
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::ControlReg ||
        Form == DecodeForm::SysXWriteImm) {
      if (Reg != 0 || Bytes.size() < PayloadOffset + 2)
        return Fail;
      const std::uint16_t Control =
          static_cast<std::uint16_t>(Bytes[PayloadOffset]) |
          (static_cast<std::uint16_t>(Bytes[PayloadOffset + 1]) << 8);
      const MCRegister GPR = decodeRegister(RM);
      if (Inst.getOpcode() == SeaBird::WRCR ||
          Form == DecodeForm::SysXWriteImm) {
        Inst.addOperand(MCOperand::createImm(Control));
        Inst.addOperand(MCOperand::createReg(GPR));
      } else {
        Inst.addOperand(MCOperand::createReg(GPR));
        Inst.addOperand(MCOperand::createImm(Control));
      }
      Size = PayloadOffset + 2;
      return Success;
    }

    if (Form == DecodeForm::TernaryRI ||
        Form == DecodeForm::TernaryByte) {
      const unsigned ImmediateBytes =
          Form == DecodeForm::TernaryByte ? 1 : (Is64Bit ? 8 : 4);
      if (Bytes.size() < PayloadOffset + ImmediateBytes)
        return Fail;
      std::uint64_t Immediate = 0;
      for (unsigned I = 0; I < ImmediateBytes; ++I)
        Immediate |= std::uint64_t(Bytes[PayloadOffset + I]) << (I * 8);
      const bool VectorShift = Inst.getOpcode() == SeaBird::VSHL128 ||
                               Inst.getOpcode() == SeaBird::VSHR128;
      if (VectorShift != HasVectorCtl)
        return Fail;
      Inst.addOperand(MCOperand::createReg(
          VectorShift ? decodeVector(Reg) : decodeRegister(Reg)));
      Inst.addOperand(MCOperand::createReg(
          VectorShift ? decodeVector(RM) : decodeRegister(RM)));
      Inst.addOperand(MCOperand::createImm(
          Form == DecodeForm::TernaryRI && !Is64Bit
              ? static_cast<std::int64_t>(
                    static_cast<std::int32_t>(Immediate))
              : static_cast<std::int64_t>(Immediate)));
      Size = PayloadOffset + ImmediateBytes;
      return Success;
    }

    if (Form == DecodeForm::AVXRegImm) {
      if (!HasVectorCtl || Bytes.size() <= PayloadOffset)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeVector(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeVector(RM)));
      Inst.addOperand(MCOperand::createImm(Bytes[PayloadOffset++]));
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::AVXBinaryImm) {
      if (!HasVectorCtl || Bytes.size() < PayloadOffset + 2)
        return Fail;
      const std::uint8_t XOP = Bytes[PayloadOffset++];
      if ((XOP >> 5) != 1)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeVector(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeVector(RM)));
      Inst.addOperand(MCOperand::createReg(decodeVector(XOP & 31)));
      Inst.addOperand(MCOperand::createImm(Bytes[PayloadOffset++]));
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::TernaryRR) {
      if (HasVectorCtl || Bytes.size() <= PayloadOffset ||
          Bytes[PayloadOffset] > 31)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeRegister(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
      Inst.addOperand(
          MCOperand::createReg(decodeRegister(Bytes[PayloadOffset])));
      Size = PayloadOffset + 1;
      return Success;
    }

    if (Form == DecodeForm::DSPTernary ||
        Form == DecodeForm::DSPQuaternary) {
      const unsigned ExtraCount =
          Form == DecodeForm::DSPTernary ? 1 : 2;
      if (HasVectorCtl || Bytes.size() < PayloadOffset + ExtraCount)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeRegister(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
      for (unsigned I = 0; I < ExtraCount; ++I) {
        const std::uint8_t Extra = Bytes[PayloadOffset++];
        if (Extra > 31)
          return Fail;
        Inst.addOperand(MCOperand::createReg(decodeRegister(Extra)));
      }
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::DSPDot ||
        Form == DecodeForm::DSPSumDot) {
      if (!HasVectorCtl)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeRegister(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeVector(RM)));
      if (Form == DecodeForm::DSPDot) {
        if (Bytes.size() <= PayloadOffset)
          return Fail;
        const std::uint8_t Extra = Bytes[PayloadOffset++];
        if ((Extra >> 5) != 1)
          return Fail;
        Inst.addOperand(MCOperand::createReg(decodeVector(Extra & 31)));
      }
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::FPBinary || Form == DecodeForm::VBinary) {
      if (!HasVectorCtl || Bytes.size() <= PayloadOffset)
        return Fail;
      const std::uint8_t XOP = Bytes[PayloadOffset++];
      if ((XOP >> 5) != 1)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeVector(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeVector(RM)));
      Inst.addOperand(MCOperand::createReg(decodeVector(XOP & 31)));
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::FPXFused) {
      if (!HasVectorCtl || Bytes.size() < PayloadOffset + 2)
        return Fail;
      const std::uint8_t MulRHS = Bytes[PayloadOffset++];
      const std::uint8_t Addend = Bytes[PayloadOffset++];
      if ((MulRHS >> 5) != 1 || (Addend >> 5) != 1)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeVector(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeVector(RM)));
      Inst.addOperand(MCOperand::createReg(decodeVector(MulRHS & 31)));
      Inst.addOperand(MCOperand::createReg(decodeVector(Addend & 31)));
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::CopyVV) {
      if (!HasVectorCtl)
        return Fail;
      Inst.addOperand(MCOperand::createReg(decodeVector(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeVector(RM)));
      Size = PayloadOffset;
      return Success;
    }

    if (Form == DecodeForm::IntToFP) {
      Inst.addOperand(MCOperand::createReg(decodeVector(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
      Size = PayloadOffset;
      return Success;
    }
    if (Form == DecodeForm::FPToInt) {
      Inst.addOperand(MCOperand::createReg(decodeRegister(Reg)));
      Inst.addOperand(MCOperand::createReg(decodeVector(RM)));
      Size = PayloadOffset;
      return Success;
    }

    if (MemoryForm) {
      if (!HasSIB && Mod == 0 && (RM & 7) == 5)
        return Fail;
      std::int64_t Disp = 0;
      if (Mod == 1) {
        if (Bytes.size() <= PayloadOffset)
          return Fail;
        Disp = static_cast<std::int8_t>(Bytes[PayloadOffset++]);
      } else if (Mod == 2) {
        if (Bytes.size() < PayloadOffset + 4)
          return Fail;
        std::uint32_t Raw = 0;
        for (unsigned I = 0; I < 4; ++I)
          Raw |= std::uint32_t(Bytes[PayloadOffset++]) << (I * 8);
        Disp = static_cast<std::int32_t>(Raw);
      }
      const bool Load = Form == DecodeForm::LoadQ ||
                        Form == DecodeForm::ScalarFPLoad ||
                        Form == DecodeForm::AddressCalc ||
                        Form == DecodeForm::VectorLoad;
      const bool ScalarFP = Form == DecodeForm::ScalarFPLoad ||
                            Form == DecodeForm::ScalarFPStore;
      const bool VectorMemory = Form == DecodeForm::VectorLoad ||
                                Form == DecodeForm::VectorStore ||
                                Form == DecodeForm::VectorAtomic ||
                                Form == DecodeForm::AVXGather ||
                                Form == DecodeForm::AVXScatter;
      if (Form == DecodeForm::AVXGather) {
        Inst.addOperand(MCOperand::createReg(decodeVector(Reg)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Inst.addOperand(MCOperand::createImm(AVXMask));
        Size = PayloadOffset;
        return Success;
      }
      if (Form == DecodeForm::AVXScatter) {
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Inst.addOperand(MCOperand::createReg(decodeVector(Reg)));
        Inst.addOperand(MCOperand::createImm(AVXMask));
        Size = PayloadOffset;
        return Success;
      }
      if (Form == DecodeForm::SysXMemory) {
        if (Reg != 0)
          return Fail;
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Size = PayloadOffset;
        return Success;
      }
      if (Form == DecodeForm::AtomicMem ||
          Form == DecodeForm::AtomicLoad) {
        const MCRegister Dst = decodeRegister(Reg);
        Inst.addOperand(MCOperand::createReg(Dst));
        if (Form == DecodeForm::AtomicMem)
          Inst.addOperand(MCOperand::createReg(Dst));
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Size = PayloadOffset;
        return Success;
      }
      if (Form == DecodeForm::MemoryOnly) {
        if (Reg != 0)
          return Fail;
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Size = PayloadOffset;
        return Success;
      }
      if (Form == DecodeForm::PairStore) {
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Reg)));
        Inst.addOperand(
            MCOperand::createReg(decodeRegister(PairStoreExtra)));
        Size = PayloadOffset;
        return Success;
      }
      if (Form == DecodeForm::AtomicCAS) {
        const MCRegister Expected = decodeRegister(Reg);
        Inst.addOperand(MCOperand::createReg(Expected));
        Inst.addOperand(MCOperand::createReg(Expected));
        Inst.addOperand(
            MCOperand::createReg(decodeRegister(AtomicCASDesired)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Size = PayloadOffset;
        return Success;
      }
      if (Form == DecodeForm::VectorAtomic) {
        if (!HasVectorCtl)
          return Fail;
        const MCRegister Value = decodeVector(Reg);
        Inst.addOperand(MCOperand::createReg(Value));
        Inst.addOperand(MCOperand::createReg(Value));
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Size = PayloadOffset;
        return Success;
      }
      if (Load) {
        Inst.addOperand(MCOperand::createReg(
            (ScalarFP || VectorMemory) ? decodeVector(Reg) : decodeRegister(Reg)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
      } else {
        Inst.addOperand(MCOperand::createReg(decodeRegister(RM)));
        Inst.addOperand(MCOperand::createReg(decodeRegister(Index)));
        Inst.addOperand(MCOperand::createImm(Scale));
        Inst.addOperand(MCOperand::createImm(Disp));
        Inst.addOperand(MCOperand::createReg(
            (ScalarFP || VectorMemory) ? decodeVector(Reg) : decodeRegister(Reg)));
      }
      Size = PayloadOffset;
      return Success;
    }

    const MCRegister Dst = decodeRegister(Reg);
    const MCRegister Src = decodeRegister(RM);
    Inst.addOperand(MCOperand::createReg(Dst));
    if (Form == DecodeForm::ALURR)
      Inst.addOperand(MCOperand::createReg(Dst));
    Inst.addOperand(MCOperand::createReg(Src));
    Size = PayloadOffset;
    return Success;
  }
};

MCDisassembler *createSeaBirdDisassembler(const Target &T,
                                           const MCSubtargetInfo &STI,
                                           MCContext &Context) {
  return new SeaBirdDisassembler(STI, Context);
}

} // namespace

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeSeaBirdDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheSeaBird32Target(),
                                         createSeaBirdDisassembler);
  TargetRegistry::RegisterMCDisassembler(getTheSeaBird64Target(),
                                         createSeaBirdDisassembler);
}
