#include "SeaBirdBaseInfo.h"
#include "SeaBirdFixupKinds.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/MathExtras.h"

#include <cstdint>

using namespace llvm;

namespace {

class SeaBirdMCCodeEmitter final : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Context;

  static unsigned registerNumber(MCRegister Reg) {
    assert(Reg >= SeaBird::R0 && Reg <= SeaBird::R31);
    return Reg.id() - SeaBird::R0;
  }

  static unsigned vectorNumber(MCRegister Reg) {
    assert(Reg >= SeaBird::V0 && Reg <= SeaBird::V31);
    return Reg.id() - SeaBird::V0;
  }

  static void emitByte(SmallVectorImpl<char> &Code, std::uint8_t Byte) {
    Code.push_back(static_cast<char>(Byte));
  }

  static void emitLittleEndian(SmallVectorImpl<char> &Code,
                               std::uint64_t Value, unsigned Bytes) {
    for (unsigned I = 0; I < Bytes; ++I)
      emitByte(Code, static_cast<std::uint8_t>(Value >> (I * 8)));
  }

public:
  SeaBirdMCCodeEmitter(const MCInstrInfo &MCII, MCContext &Context)
      : MCII(MCII), Context(Context) {}

  void encodeInstruction(const MCInst &Inst, SmallVectorImpl<char> &Code,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override {
    const std::uint64_t Flags = MCII.get(Inst.getOpcode()).TSFlags;
    const unsigned Marker = Inst.getFlags() & SeaBirdII::MarkerMask;
    if (Marker != SeaBirdII::NoMarker) {
      if (Marker > SeaBirdII::Leaf) {
        Context.reportError(Inst.getLoc(), "unknown SeaBird performance marker");
        return;
      }
      emitByte(Code, SeaBirdII::PerformanceMarkerEscape);
      emitByte(Code, static_cast<std::uint8_t>(Marker));
    }
    const unsigned Opcode = Flags & SeaBirdII::OpcodeMask;
    const unsigned Form = (Flags & SeaBirdII::FormMask) >> SeaBirdII::FormShift;
    const unsigned ExtensionGroup =
        (Flags & SeaBirdII::ExtensionGroupMask) >>
        SeaBirdII::ExtensionGroupShift;
    const unsigned ScalarWidth =
        (Flags & SeaBirdII::ScalarWidthMask) >> SeaBirdII::ScalarWidthShift;
    const std::uint8_t ScalarVectorCtl =
        ScalarWidth ? static_cast<std::uint8_t>((ScalarWidth - 1) << 3)
                    : 0x18;
    const std::uint8_t ScalarOperandControl =
        ScalarWidth ? static_cast<std::uint8_t>(ScalarWidth << 3) : 0x20;

    if (ExtensionGroup != 0) {
      if (ExtensionGroup == 1) {
        auto EmitAVXOpcode = [&]() {
          emitByte(Code, 0xFF);
          emitByte(Code, ExtensionGroup);
          emitByte(Code, Opcode);
        };

        if (Form == SeaBirdII::NoOperand) {
          EmitAVXOpcode();
          return;
        }

        if (Form == SeaBirdII::VectorLoad ||
            Form == SeaBirdII::VectorStore) {
          const bool Gather = Form == SeaBirdII::VectorLoad;
          const unsigned MemoryOp = Gather ? 1 : 0;
          const unsigned RegOp = Gather ? 0 : 4;
          const unsigned MaskOp = Gather ? 5 : 5;
          const unsigned Reg = vectorNumber(Inst.getOperand(RegOp).getReg());
          const unsigned Base =
              registerNumber(Inst.getOperand(MemoryOp).getReg());
          const MCRegister IndexReg = Inst.getOperand(MemoryOp + 1).getReg();
          const bool HasIndex = IndexReg != SeaBird::R4 &&
                                IndexReg != SeaBird::NOIDX;
          const unsigned Index = HasIndex ? registerNumber(IndexReg) : 4;
          const std::int64_t Scale = Inst.getOperand(MemoryOp + 2).getImm();
          const std::int64_t Disp = Inst.getOperand(MemoryOp + 3).getImm();
          const MCOperand &Mask = Inst.getOperand(MaskOp);
          if (!Mask.isImm() || !isUInt<8>(Mask.getImm())) {
            Context.reportError(Inst.getLoc(),
                                "SeaBird AVX mask must fit in 8 bits");
            return;
          }
          if (Scale != 1 && Scale != 2 && Scale != 4 && Scale != 8) {
            Context.reportError(Inst.getLoc(),
                                "SeaBird AVX memory scale must be 1, 2, 4, or 8");
            return;
          }
          if (!isInt<32>(Disp)) {
            Context.reportError(
                Inst.getLoc(), "SeaBird AVX memory displacement out of range");
            return;
          }
          const bool SIB = HasIndex || (Base & 7) == 4;
          const unsigned Mod =
              (Disp != 0 || (Base & 7) == 5)
                  ? (isInt<8>(Disp) ? 1 : 2)
                  : 0;
          const bool Extended =
              Reg > 7 || Base > 7 || (HasIndex && Index > 7);
          emitByte(Code, 0xFE);
          emitByte(Code,
                   static_cast<std::uint8_t>(0x40 |
                                             (Extended ? 0x80 : 0)));
          EmitAVXOpcode();
          emitByte(Code, static_cast<std::uint8_t>(
                             (Mod << 6) | ((Reg & 7) << 3) |
                             (SIB ? 4 : (Base & 7))));
          if (SIB) {
            const unsigned ScaleBits =
                Scale == 1 ? 0 : Scale == 2 ? 1 : Scale == 4 ? 2 : 3;
            emitByte(Code, static_cast<std::uint8_t>(
                               (ScaleBits << 6) | ((Index & 7) << 3) |
                               (Base & 7)));
          }
          if (Extended)
            emitByte(Code, static_cast<std::uint8_t>(
                               ((Reg >> 3) & 3) |
                               (SIB ? (((Index >> 3) & 3) << 4) |
                                          (((Base >> 3) & 3) << 6)
                                    : (((Base >> 3) & 3) << 2))));
          emitByte(Code, 0x18);
          emitByte(Code, static_cast<std::uint8_t>(Mask.getImm()));
          if (Mod == 1)
            emitByte(Code, static_cast<std::uint8_t>(Disp));
          else if (Mod == 2)
            emitLittleEndian(Code, static_cast<std::uint32_t>(Disp), 4);
          return;
        }

        const unsigned Dst = vectorNumber(Inst.getOperand(0).getReg());
        const unsigned Src = vectorNumber(Inst.getOperand(1).getReg());
        const bool Extended = Dst > 7 || Src > 7;
        emitByte(Code, 0xFE);
        emitByte(Code,
                 static_cast<std::uint8_t>(0x40 |
                                           (Extended ? 0x80 : 0)));
        EmitAVXOpcode();
        emitByte(Code, static_cast<std::uint8_t>(
                           0xC0 | ((Dst & 7) << 3) | (Src & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(
                             ((Dst >> 3) & 3) | (((Src >> 3) & 3) << 2)));
        emitByte(Code, 0x18);

        if (Form == SeaBirdII::VBinary ||
            Form == SeaBirdII::FPBinary) {
          const unsigned Rhs = vectorNumber(Inst.getOperand(2).getReg());
          emitByte(Code, static_cast<std::uint8_t>(0x20 | Rhs));
        }

        if (Form == SeaBirdII::TernaryByte ||
            Form == SeaBirdII::FPBinary) {
          const MCOperand &Immediate =
              Inst.getOperand(Inst.getNumOperands() - 1);
          if (!Immediate.isImm() || !isUInt<8>(Immediate.getImm())) {
            Context.reportError(Inst.getLoc(),
                                "SeaBird AVX immediate must fit in 8 bits");
            return;
          }
          emitByte(Code, static_cast<std::uint8_t>(Immediate.getImm()));
        }
        return;
      }

      if (ExtensionGroup == 3) {
        const bool VectorDSP = Inst.getOpcode() == SeaBird::DOTP ||
                               Inst.getOpcode() == SeaBird::SUMDOTP;
        const unsigned Dst = registerNumber(Inst.getOperand(0).getReg());
        const unsigned Src =
            VectorDSP ? vectorNumber(Inst.getOperand(1).getReg())
                      : registerNumber(Inst.getOperand(1).getReg());
        const bool Extended = Dst > 7 || Src > 7;
        if (Extended || VectorDSP) {
          emitByte(Code, 0xFE);
          emitByte(Code, static_cast<std::uint8_t>(
                             (Extended ? 0x80 : 0) |
                             (VectorDSP ? 0x40 : 0)));
        }
        emitByte(Code, 0xFF);
        emitByte(Code, ExtensionGroup);
        emitByte(Code, Opcode);
        emitByte(Code, static_cast<std::uint8_t>(
                           0xC0 | ((Dst & 7) << 3) | (Src & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(
                             ((Dst >> 3) & 3) | (((Src >> 3) & 3) << 2)));
        if (VectorDSP)
          emitByte(Code, 0x18);
        if (Form == SeaBirdII::TernaryByte) {
          const MCOperand &Immediate = Inst.getOperand(2);
          if (!Immediate.isImm() || !isUInt<8>(Immediate.getImm())) {
            Context.reportError(Inst.getLoc(),
                                "SeaBird DSP immediate must fit in 8 bits");
            return;
          }
          emitByte(Code, static_cast<std::uint8_t>(Immediate.getImm()));
          return;
        }
        for (unsigned I = 2; I < Inst.getNumOperands(); ++I) {
          const unsigned Extra =
              VectorDSP ? vectorNumber(Inst.getOperand(I).getReg())
                        : registerNumber(Inst.getOperand(I).getReg());
          emitByte(Code, static_cast<std::uint8_t>(
                             (VectorDSP ? 0x20 : 0) | Extra));
        }
        return;
      }

      if (ExtensionGroup == 2) {
        const unsigned Dst = vectorNumber(Inst.getOperand(0).getReg());
        const unsigned Src = vectorNumber(Inst.getOperand(1).getReg());
        const bool Extended = Dst > 7 || Src > 7;
        emitByte(Code, 0xFE);
        emitByte(Code,
                 static_cast<std::uint8_t>(0x40 | (Extended ? 0x80 : 0)));
        emitByte(Code, 0xFF);
        emitByte(Code, ExtensionGroup);
        emitByte(Code, Opcode);
        emitByte(Code, static_cast<std::uint8_t>(
                           0xC0 | ((Dst & 7) << 3) | (Src & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(
                             ((Dst >> 3) & 3) | (((Src >> 3) & 3) << 2)));
        emitByte(Code, 0x18);
        for (unsigned I = 2; I < Inst.getNumOperands(); ++I) {
          const unsigned Extra = vectorNumber(Inst.getOperand(I).getReg());
          emitByte(Code, static_cast<std::uint8_t>(0x20 | Extra));
        }
        return;
      }

      if (ExtensionGroup == 5) {
        if (Form == SeaBirdII::ScalarFPLoad ||
            Form == SeaBirdII::ScalarFPStore) {
          const bool Load = Form == SeaBirdII::ScalarFPLoad;
          const unsigned MemoryOp = Load ? 1 : 0;
          const unsigned RegOp = Load ? 0 : 4;
          const unsigned Reg = vectorNumber(Inst.getOperand(RegOp).getReg());
          const unsigned Base =
              registerNumber(Inst.getOperand(MemoryOp).getReg());
          const MCRegister IndexReg = Inst.getOperand(MemoryOp + 1).getReg();
          const bool HasIndex = IndexReg != SeaBird::R4 &&
                                IndexReg != SeaBird::NOIDX;
          const unsigned Index = HasIndex ? registerNumber(IndexReg) : 4;
          const std::int64_t Scale = Inst.getOperand(MemoryOp + 2).getImm();
          const std::int64_t Disp = Inst.getOperand(MemoryOp + 3).getImm();
          const bool SIB = HasIndex || (Base & 7) == 4;
          unsigned Mod = 0;
          if (Disp != 0 || (Base & 7) == 5)
            Mod = isInt<8>(Disp) ? 1 : 2;
          if (!isInt<32>(Disp)) {
            Context.reportError(
                Inst.getLoc(), "SeaBird FP memory displacement out of range");
            return;
          }
          const bool Extended =
              Reg > 7 || Base > 7 || (HasIndex && Index > 7);
          emitByte(Code, 0xFE);
          emitByte(Code,
                   static_cast<std::uint8_t>(ScalarOperandControl |
                                             (Extended ? 0x80 : 0)));
          emitByte(Code, 0xFF);
          emitByte(Code, ExtensionGroup);
          emitByte(Code, Opcode);
          emitByte(Code, static_cast<std::uint8_t>(
                             (Mod << 6) | ((Reg & 7) << 3) |
                             (SIB ? 4 : (Base & 7))));
          if (SIB) {
            const unsigned ScaleBits =
                Scale == 1 ? 0 : Scale == 2 ? 1 : Scale == 4 ? 2 : 3;
            emitByte(Code, static_cast<std::uint8_t>(
                               (ScaleBits << 6) | ((Index & 7) << 3) |
                               (Base & 7)));
          }
          if (Extended)
            emitByte(Code, static_cast<std::uint8_t>(
                               ((Reg >> 3) & 3) |
                               (SIB ? (((Index >> 3) & 3) << 4) |
                                          (((Base >> 3) & 3) << 6)
                                    : (((Base >> 3) & 3) << 2))));
          if (Mod == 1)
            emitByte(Code, static_cast<std::uint8_t>(Disp));
          else if (Mod == 2)
            emitLittleEndian(Code, static_cast<std::uint32_t>(Disp), 4);
          return;
        }

        const unsigned Dst = vectorNumber(Inst.getOperand(0).getReg());
        const unsigned Src = vectorNumber(Inst.getOperand(1).getReg());
        const bool Extended = Dst > 7 || Src > 7;
        emitByte(Code, 0xFE);
        emitByte(Code,
                 static_cast<std::uint8_t>(0x40 | (Extended ? 0x80 : 0)));
        emitByte(Code, 0xFF);
        emitByte(Code, ExtensionGroup);
        emitByte(Code, Opcode);
        emitByte(Code, static_cast<std::uint8_t>(
                           0xC0 | ((Dst & 7) << 3) | (Src & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(
                             ((Dst >> 3) & 3) | (((Src >> 3) & 3) << 2)));
        emitByte(Code, ScalarVectorCtl);
        for (unsigned I = 2; I < Inst.getNumOperands(); ++I) {
          const unsigned Extra = vectorNumber(Inst.getOperand(I).getReg());
          emitByte(Code, static_cast<std::uint8_t>(0x20 | Extra));
        }
        return;
      }

      if (ExtensionGroup != 4) {
        Context.reportError(Inst.getLoc(),
                            "unsupported SeaBird extension group");
        return;
      }

      auto EmitSysXOpcode = [&]() {
        emitByte(Code, 0xFF);
        emitByte(Code, ExtensionGroup);
        emitByte(Code, Opcode);
      };

      if (Form == SeaBirdII::NoOperand) {
        EmitSysXOpcode();
        return;
      }

      if (Inst.getOpcode() == SeaBird::SETMODE) {
        const MCOperand &Immediate = Inst.getOperand(0);
        if (!Immediate.isImm() || !isUInt<8>(Immediate.getImm())) {
          Context.reportError(Inst.getLoc(),
                              "SeaBird SYSX mode must fit in 8 bits");
          return;
        }
        EmitSysXOpcode();
        emitByte(Code, static_cast<std::uint8_t>(Immediate.getImm()));
        return;
      }

      if (Inst.getOpcode() == SeaBird::IN ||
          Inst.getOpcode() == SeaBird::VMREAD ||
          Inst.getOpcode() == SeaBird::RDPMC) {
        const unsigned RM = registerNumber(Inst.getOperand(0).getReg());
        const MCOperand &Selector = Inst.getOperand(1);
        if (!Selector.isImm() || !isUInt<16>(Selector.getImm())) {
          Context.reportError(Inst.getLoc(),
                              "SeaBird SYSX selector must fit in 16 bits");
          return;
        }
        const bool Extended = RM > 7;
        if (Extended) {
          emitByte(Code, 0xFE);
          emitByte(Code, 0x80);
        }
        EmitSysXOpcode();
        emitByte(Code, static_cast<std::uint8_t>(0xC0 | (RM & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(((RM >> 3) & 3) << 2));
        emitLittleEndian(Code, static_cast<std::uint64_t>(Selector.getImm()), 2);
        return;
      }

      if (Inst.getOpcode() == SeaBird::OUT ||
          Inst.getOpcode() == SeaBird::VMWRITE) {
        const MCOperand &Selector = Inst.getOperand(0);
        const unsigned RM = registerNumber(Inst.getOperand(1).getReg());
        if (!Selector.isImm() || !isUInt<16>(Selector.getImm())) {
          Context.reportError(Inst.getLoc(),
                              "SeaBird SYSX selector must fit in 16 bits");
          return;
        }
        const bool Extended = RM > 7;
        if (Extended) {
          emitByte(Code, 0xFE);
          emitByte(Code, 0x80);
        }
        EmitSysXOpcode();
        emitByte(Code, static_cast<std::uint8_t>(0xC0 | (RM & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(((RM >> 3) & 3) << 2));
        emitLittleEndian(Code, static_cast<std::uint64_t>(Selector.getImm()), 2);
        return;
      }

      if (Inst.getOpcode() == SeaBird::QUERY ||
          Inst.getOpcode() == SeaBird::INVTLB) {
        const unsigned Reg = registerNumber(Inst.getOperand(0).getReg());
        const unsigned RM = registerNumber(Inst.getOperand(1).getReg());
        const bool Extended = Reg > 7 || RM > 7;
        if (Extended) {
          emitByte(Code, 0xFE);
          emitByte(Code, 0x80);
        }
        EmitSysXOpcode();
        emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Reg & 7) << 3) |
                                                 (RM & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(((Reg >> 3) & 3) |
                                                   (((RM >> 3) & 3) << 2)));
        return;
      }

      if (Inst.getOpcode() == SeaBird::EOI ||
          Inst.getOpcode() == SeaBird::GETCPL ||
          Inst.getOpcode() == SeaBird::INVTLBASID ||
          Inst.getOpcode() == SeaBird::RNGGET) {
        const unsigned Reg = registerNumber(Inst.getOperand(0).getReg());
        const bool Extended = Reg > 7;
        if (Extended) {
          emitByte(Code, 0xFE);
          emitByte(Code, 0x80);
        }
        EmitSysXOpcode();
        emitByte(Code, static_cast<std::uint8_t>(0xC0 | (Reg & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(((Reg >> 3) & 3) << 2));
        return;
      }

      if (Inst.getOpcode() == SeaBird::SENDIPI) {
        const unsigned Reg = registerNumber(Inst.getOperand(0).getReg());
        const unsigned RM = registerNumber(Inst.getOperand(1).getReg());
        const unsigned Extra = registerNumber(Inst.getOperand(2).getReg());
        const bool Extended = Reg > 7 || RM > 7;
        if (Extended) {
          emitByte(Code, 0xFE);
          emitByte(Code, 0x80);
        }
        EmitSysXOpcode();
        emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Reg & 7) << 3) |
                                                 (RM & 7)));
        if (Extended)
          emitByte(Code, static_cast<std::uint8_t>(((Reg >> 3) & 3) |
                                                   (((RM >> 3) & 3) << 2)));
        emitByte(Code, static_cast<std::uint8_t>(Extra));
        return;
      }

      const unsigned Base = registerNumber(Inst.getOperand(0).getReg());
      const MCRegister IndexReg = Inst.getOperand(1).getReg();
      const bool HasIndex = IndexReg != SeaBird::R4 &&
                            IndexReg != SeaBird::NOIDX;
      const unsigned Index = HasIndex ? registerNumber(IndexReg) : 4;
      const std::int64_t Scale = Inst.getOperand(2).getImm();
      const std::int64_t Disp = Inst.getOperand(3).getImm();
      const bool SIB = HasIndex || (Base & 7) == 4;
      const unsigned Mod =
          (Disp != 0 || (Base & 7) == 5) ? (isInt<8>(Disp) ? 1 : 2) : 0;
      const bool HasRegField = Inst.getOpcode() == SeaBird::XSAVE ||
                               Inst.getOpcode() == SeaBird::XRSTOR ||
                               Inst.getOpcode() == SeaBird::WRSS;
      const unsigned Reg =
          HasRegField ? registerNumber(Inst.getOperand(4).getReg()) : 0;
      if (!isInt<32>(Disp)) {
        Context.reportError(Inst.getLoc(),
                            "SeaBird SYSX memory displacement out of range");
        return;
      }
      const bool Extended = Reg > 7 || Base > 7 || (HasIndex && Index > 7);
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      EmitSysXOpcode();
      emitByte(Code, static_cast<std::uint8_t>((Mod << 6) | ((Reg & 7) << 3) |
                                               (SIB ? 4 : (Base & 7))));
      if (SIB) {
        const unsigned ScaleBits =
            Scale == 1 ? 0 : Scale == 2 ? 1 : Scale == 4 ? 2 : 3;
        emitByte(Code, static_cast<std::uint8_t>((ScaleBits << 6) |
                                                 ((Index & 7) << 3) |
                                                 (Base & 7)));
      }
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(
                           ((Reg >> 3) & 3) |
                           (SIB ? (((Index >> 3) & 3) << 4) |
                                     (((Base >> 3) & 3) << 6)
                                : (((Base >> 3) & 3) << 2))));
      if (Mod == 1)
        emitByte(Code, static_cast<std::uint8_t>(Disp));
      else if (Mod == 2)
        emitLittleEndian(Code, static_cast<std::uint32_t>(Disp), 4);
      return;
    }

    if (Form == SeaBirdII::NoOperand) {
      emitByte(Code, Opcode);
      return;
    }

    if (Form == SeaBirdII::TrapImm) {
      emitByte(Code, Opcode);
      const MCOperand &Immediate = Inst.getOperand(0);
      if (!Immediate.isImm() || !isUInt<8>(Immediate.getImm())) {
        Context.reportError(Inst.getLoc(),
                            "SeaBird trap vector must fit in 8 bits");
        emitByte(Code, 0);
        return;
      }
      emitByte(Code, static_cast<std::uint8_t>(Immediate.getImm()));
      return;
    }

    if (Form == SeaBirdII::FrameImm) {
      emitByte(Code, Opcode);
      const unsigned ImmediateBytes =
          STI.getTargetTriple().isArch64Bit() ? 8 : 4;
      const MCOperand &Immediate = Inst.getOperand(0);
      const bool InRange =
          Immediate.isImm() && Immediate.getImm() >= 0 &&
          (ImmediateBytes == 8 ||
           isUInt<32>(static_cast<std::uint64_t>(Immediate.getImm())));
      if (!InRange) {
        Context.reportError(
            Inst.getLoc(),
            "SeaBird frame size must fit the unsigned address width");
        emitLittleEndian(Code, 0, ImmediateBytes);
        return;
      }
      emitLittleEndian(Code, static_cast<std::uint64_t>(Immediate.getImm()),
                       ImmediateBytes);
      return;
    }

    if (Form == SeaBirdII::BranchRel32) {
      emitByte(Code, Opcode);
      const MCOperand &Target = Inst.getOperand(0);
      if (Target.isImm()) {
        if (!isInt<32>(Target.getImm())) {
          Context.reportError(Inst.getLoc(), "SeaBird rel32 target out of range");
          emitLittleEndian(Code, 0, 4);
          return;
        }
        emitLittleEndian(Code, static_cast<std::uint32_t>(Target.getImm()), 4);
      } else {
        const MCExpr *Bias = MCConstantExpr::create(4, Context);
        const MCExpr *Expr =
            MCBinaryExpr::createSub(Target.getExpr(), Bias, Context);
        // The marker prefix, when present, precedes the opcode.  Anchor the
        // relocation at the current output position instead of assuming the
        // displacement always begins at byte one.
        Fixups.push_back(MCFixup::create(
            Code.size(), Expr, MCFixupKind(SeaBird::fixup_seabird_pcrel32),
            true));
        emitLittleEndian(Code, 0, 4);
      }
      return;
    }

    if (Form == SeaBirdII::RI64 || Form == SeaBirdII::ALURI ||
        Form == SeaBirdII::CompareRI) {
      const unsigned ImmediateBytes =
          STI.getTargetTriple().isArch64Bit() ? 8 : 4;
      const unsigned Dst = registerNumber(Inst.getOperand(0).getReg());
      const bool Extended = Dst > 7;
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | (Dst & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Dst >> 3) & 3) << 2));
      const MCOperand &Immediate =
          Inst.getOperand(Inst.getNumOperands() - 1);
      if (Immediate.isExpr()) {
        Fixups.push_back(MCFixup::create(
            Code.size(), Immediate.getExpr(),
            ImmediateBytes == 8 ? FK_Data_8 : FK_Data_4, true));
        emitLittleEndian(Code, 0, ImmediateBytes);
        return;
      }
      if (!Immediate.isImm()) {
        Context.reportError(Inst.getLoc(), "invalid SeaBird immediate");
        emitLittleEndian(Code, 0, ImmediateBytes);
        return;
      }
      emitLittleEndian(Code, static_cast<std::uint64_t>(Immediate.getImm()),
                       ImmediateBytes);
      return;
    }

    if (Form == SeaBirdII::IndirectCall) {
      const unsigned Target = registerNumber(Inst.getOperand(0).getReg());
      const bool Extended = Target > 7;
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | (Target & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Target >> 3) & 3) << 2));
      return;
    }

    if (Form == SeaBirdII::RegCondBranch) {
      const unsigned Reg = registerNumber(Inst.getOperand(0).getReg());
      const bool Extended = Reg > 7;
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | (Reg & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Reg >> 3) & 3) << 2));
      const MCOperand &Target = Inst.getOperand(1);
      if (Target.isImm()) {
        if (!isInt<32>(Target.getImm())) {
          Context.reportError(Inst.getLoc(),
                              "SeaBird register branch target out of range");
          emitLittleEndian(Code, 0, 4);
          return;
        }
        emitLittleEndian(Code, static_cast<std::uint32_t>(Target.getImm()), 4);
      } else {
        const MCExpr *Bias = MCConstantExpr::create(4, Context);
        const MCExpr *Expr =
            MCBinaryExpr::createSub(Target.getExpr(), Bias, Context);
        Fixups.push_back(MCFixup::create(
            Code.size(), Expr, MCFixupKind(SeaBird::fixup_seabird_pcrel32),
            true));
        emitLittleEndian(Code, 0, 4);
      }
      return;
    }

    if (Form == SeaBirdII::UnaryInPlace || Form == SeaBirdII::StackReg ||
        Form == SeaBirdII::ReadReg) {
      const unsigned Dst = registerNumber(Inst.getOperand(0).getReg());
      if ((Inst.getOpcode() == SeaBird::PUSHQ ||
           Inst.getOpcode() == SeaBird::POPQ) &&
          (Dst & 1 || Dst == 31)) {
        Context.reportError(
            Inst.getLoc(),
            "SeaBird register pair must start at an even register");
        return;
      }
      const bool Extended = Dst > 7;
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | (Dst & 7)));
      if (Extended)
        emitByte(Code,
                 static_cast<std::uint8_t>(((Dst >> 3) & 3) << 2));
      return;
    }

    if (Form == SeaBirdII::ControlReg) {
      const bool IsWrite = Inst.getOpcode() == SeaBird::WRCR;
      const unsigned RegOp = IsWrite ? 1 : 0;
      const unsigned ControlOp = IsWrite ? 0 : 1;
      const unsigned Reg =
          registerNumber(Inst.getOperand(RegOp).getReg());
      const bool Extended = Reg > 7;
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | (Reg & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Reg >> 3) & 3) << 2));
      const MCOperand &Control = Inst.getOperand(ControlOp);
      if (!Control.isImm() || !isUInt<16>(Control.getImm())) {
        Context.reportError(Inst.getLoc(),
                            "SeaBird control-register selector must fit in 16 bits");
        emitLittleEndian(Code, 0, 2);
        return;
      }
      emitLittleEndian(Code, static_cast<std::uint64_t>(Control.getImm()), 2);
      return;
    }

    if (Form == SeaBirdII::TernaryRI || Form == SeaBirdII::TernaryByte) {
      const bool VectorShift = Inst.getOpcode() == SeaBird::VSHL128 ||
                               Inst.getOpcode() == SeaBird::VSHR128;
      const unsigned Dst =
          VectorShift ? vectorNumber(Inst.getOperand(0).getReg())
                      : registerNumber(Inst.getOperand(0).getReg());
      const unsigned Src =
          VectorShift ? vectorNumber(Inst.getOperand(1).getReg())
                      : registerNumber(Inst.getOperand(1).getReg());
      const bool Extended = Dst > 7 || Src > 7;
      if (Extended || VectorShift) {
        emitByte(Code, 0xFE);
        emitByte(Code, static_cast<std::uint8_t>(
                           (Extended ? 0x80 : 0) |
                           (VectorShift ? 0x40 : 0)));
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Dst & 7) << 3) |
                                               (Src & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Dst >> 3) & 3) |
                                                 (((Src >> 3) & 3) << 2)));
      if (VectorShift)
        emitByte(Code, 0x18);
      const MCOperand &Immediate = Inst.getOperand(2);
      const unsigned ImmediateBytes =
          Form == SeaBirdII::TernaryByte
              ? 1
              : (STI.getTargetTriple().isArch64Bit() ? 8 : 4);
      if (!Immediate.isImm() ||
          (Form == SeaBirdII::TernaryByte &&
           !isUInt<8>(Immediate.getImm()))) {
        Context.reportError(Inst.getLoc(),
                            "SeaBird packed range must fit in 8 bits");
        emitLittleEndian(Code, 0, ImmediateBytes);
        return;
      }
      emitLittleEndian(Code, static_cast<std::uint64_t>(Immediate.getImm()),
                       ImmediateBytes);
      return;
    }

    if (Form == SeaBirdII::FPBinary &&
        (Inst.getOpcode() == SeaBird::PDEPrrr ||
         Inst.getOpcode() == SeaBird::PEXTrrr ||
         Inst.getOpcode() == SeaBird::CPYBrrr ||
         Inst.getOpcode() == SeaBird::CPYWrrr ||
         Inst.getOpcode() == SeaBird::MEMFILLrrr)) {
      const unsigned Dst = registerNumber(Inst.getOperand(0).getReg());
      const unsigned Src = registerNumber(Inst.getOperand(1).getReg());
      const unsigned Extra = registerNumber(Inst.getOperand(2).getReg());
      const bool Extended = Dst > 7 || Src > 7;
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Dst & 7) << 3) |
                                               (Src & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Dst >> 3) & 3) |
                                                 (((Src >> 3) & 3) << 2)));
      emitByte(Code, static_cast<std::uint8_t>(Extra));
      return;
    }

    if (Form == SeaBirdII::FPBinary &&
        Inst.getOpcode() == SeaBird::LDP) {
      const unsigned Dst1 = registerNumber(Inst.getOperand(0).getReg());
      const unsigned Dst2 = registerNumber(Inst.getOperand(1).getReg());
      const unsigned Base = registerNumber(Inst.getOperand(2).getReg());
      if ((Inst.getOperand(3).getReg() != SeaBird::R4 &&
           Inst.getOperand(3).getReg() != SeaBird::NOIDX) ||
          Inst.getOperand(4).getImm() != 1 ||
          Inst.getOperand(5).getImm() != 0) {
        Context.reportError(
            Inst.getLoc(),
            "SeaBird LDP XOP address must be a base register without displacement");
        return;
      }
      const bool Extended = Dst1 > 7 || Dst2 > 7;
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Dst1 & 7) << 3) |
                                               (Dst2 & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Dst1 >> 3) & 3) |
                                                 (((Dst2 >> 3) & 3) << 2)));
      emitByte(Code, static_cast<std::uint8_t>(Base));
      return;
    }

    if (Form == SeaBirdII::FPBinary || Form == SeaBirdII::VBinary) {
      const unsigned Dst = vectorNumber(Inst.getOperand(0).getReg());
      const unsigned Lhs = vectorNumber(Inst.getOperand(1).getReg());
      const unsigned Rhs = vectorNumber(Inst.getOperand(2).getReg());
      const bool Extended = Dst > 7 || Lhs > 7;
      emitByte(Code, 0xFE);
      emitByte(Code, static_cast<std::uint8_t>(0x40 | (Extended ? 0x80 : 0)));
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Dst & 7) << 3) |
                                               (Lhs & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Dst >> 3) & 3) |
                                                 (((Lhs >> 3) & 3) << 2)));
      emitByte(Code, ScalarVectorCtl);
      emitByte(Code, static_cast<std::uint8_t>(0x20 | Rhs));
      return;
    }

    if (Form == SeaBirdII::CopyVV) {
      const unsigned Dst = vectorNumber(Inst.getOperand(0).getReg());
      const unsigned Src = vectorNumber(Inst.getOperand(1).getReg());
      const bool Extended = Dst > 7 || Src > 7;
      emitByte(Code, 0xFE);
      emitByte(Code, static_cast<std::uint8_t>(0x40 | (Extended ? 0x80 : 0)));
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Dst & 7) << 3) |
                                               (Src & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Dst >> 3) & 3) |
                                                 (((Src >> 3) & 3) << 2)));
      emitByte(Code, ScalarVectorCtl);
      return;
    }

    if (Form == SeaBirdII::ScalarFPLoad || Form == SeaBirdII::ScalarFPStore) {
      const unsigned MemoryOp = Form == SeaBirdII::ScalarFPLoad ? 1 : 0;
      const unsigned RegOp = Form == SeaBirdII::ScalarFPLoad ? 0 : 4;
      const unsigned Reg = vectorNumber(Inst.getOperand(RegOp).getReg());
      const unsigned Base = registerNumber(Inst.getOperand(MemoryOp).getReg());
      const MCRegister IndexReg = Inst.getOperand(MemoryOp + 1).getReg();
      const bool HasIndex = IndexReg != SeaBird::R4 &&
                            IndexReg != SeaBird::NOIDX;
      const unsigned Index = HasIndex ? registerNumber(IndexReg) : 4;
      const std::int64_t Scale = Inst.getOperand(MemoryOp + 2).getImm();
      const std::int64_t Disp = Inst.getOperand(MemoryOp + 3).getImm();
      const bool SIB = HasIndex || (Base & 7) == 4;
      unsigned Mod = 0;
      if (Disp != 0 || (Base & 7) == 5)
        Mod = isInt<8>(Disp) ? 1 : 2;
      if (!isInt<32>(Disp)) {
        Context.reportError(Inst.getLoc(), "SeaBird FP memory displacement out of range");
        return;
      }
      const bool Extended = Reg > 7 || Base > 7 || (HasIndex && Index > 7);
      emitByte(Code, 0xFE);
      emitByte(Code, static_cast<std::uint8_t>(ScalarOperandControl |
                                               (Extended ? 0x80 : 0)));
      emitByte(Code, 0xFF);
      emitByte(Code, 0x05);
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>((Mod << 6) | ((Reg & 7) << 3) |
                                               (SIB ? 4 : (Base & 7))));
      if (SIB) {
        const unsigned ScaleBits = Scale == 1 ? 0 : Scale == 2 ? 1 :
                                   Scale == 4 ? 2 : 3;
        emitByte(Code, static_cast<std::uint8_t>((ScaleBits << 6) |
                                                 ((Index & 7) << 3) |
                                                 (Base & 7)));
      }
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(
                           ((Reg >> 3) & 3) |
                           (SIB ? (((Index >> 3) & 3) << 4) |
                                      (((Base >> 3) & 3) << 6)
                                : (((Base >> 3) & 3) << 2))));
      if (Mod == 1)
        emitByte(Code, static_cast<std::uint8_t>(Disp));
      else if (Mod == 2)
        emitLittleEndian(Code, static_cast<std::uint32_t>(Disp), 4);
      return;
    }

    if (Form == SeaBirdII::VectorLoad || Form == SeaBirdII::VectorStore) {
      const bool Load = Form == SeaBirdII::VectorLoad;
      const bool AtomicVectorRMW = Inst.getOpcode() == SeaBird::XCHG128;
      const unsigned MemoryOp = AtomicVectorRMW ? 2 : (Load ? 1 : 0);
      const unsigned RegOp = Load ? 0 : 4;
      const unsigned Reg = vectorNumber(Inst.getOperand(RegOp).getReg());
      const unsigned Base = registerNumber(Inst.getOperand(MemoryOp).getReg());
      const MCRegister IndexReg = Inst.getOperand(MemoryOp + 1).getReg();
      const bool HasIndex = IndexReg != SeaBird::R4 &&
                            IndexReg != SeaBird::NOIDX;
      const unsigned Index = HasIndex ? registerNumber(IndexReg) : 4;
      const std::int64_t Scale = Inst.getOperand(MemoryOp + 2).getImm();
      const std::int64_t Disp = Inst.getOperand(MemoryOp + 3).getImm();
      const bool SIB = HasIndex || (Base & 7) == 4;
      unsigned Mod = (Disp != 0 || (Base & 7) == 5)
                         ? (isInt<8>(Disp) ? 1 : 2) : 0;
      if (!isInt<32>(Disp)) {
        Context.reportError(Inst.getLoc(), "SeaBird vector displacement out of range");
        return;
      }
      const bool Extended = Reg > 7 || Base > 7 || (HasIndex && Index > 7);
      emitByte(Code, 0xFE);
      emitByte(Code, static_cast<std::uint8_t>(0x40 | (Extended ? 0x80 : 0)));
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>((Mod << 6) | ((Reg & 7) << 3) |
                                               (SIB ? 4 : (Base & 7))));
      if (SIB) {
        const unsigned ScaleBits = Scale == 1 ? 0 : Scale == 2 ? 1 :
                                   Scale == 4 ? 2 : 3;
        emitByte(Code, static_cast<std::uint8_t>((ScaleBits << 6) |
                                                 ((Index & 7) << 3) |
                                                 (Base & 7)));
      }
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(
                           ((Reg >> 3) & 3) |
                           (SIB ? (((Index >> 3) & 3) << 4) |
                                      (((Base >> 3) & 3) << 6)
                                : (((Base >> 3) & 3) << 2))));
      emitByte(Code, ScalarVectorCtl);
      if (Mod == 1)
        emitByte(Code, static_cast<std::uint8_t>(Disp));
      else if (Mod == 2)
        emitLittleEndian(Code, static_cast<std::uint32_t>(Disp), 4);
      return;
    }

    if (Form == SeaBirdII::IntToFP || Form == SeaBirdII::FPToInt) {
      const bool ToFP = Form == SeaBirdII::IntToFP;
      const unsigned Reg = ToFP ? vectorNumber(Inst.getOperand(0).getReg())
                                : registerNumber(Inst.getOperand(0).getReg());
      const unsigned RM = ToFP ? registerNumber(Inst.getOperand(1).getReg())
                               : vectorNumber(Inst.getOperand(1).getReg());
      const bool Extended = Reg > 7 || RM > 7;
      emitByte(Code, 0xFE);
      emitByte(Code, static_cast<std::uint8_t>(0x40 | (Extended ? 0x80 : 0)));
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Reg & 7) << 3) |
                                               (RM & 7)));
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(((Reg >> 3) & 3) |
                                                 (((RM >> 3) & 3) << 2)));
      emitByte(Code, ScalarVectorCtl);
      return;
    }

    if (Form == SeaBirdII::LoadQ || Form == SeaBirdII::StoreQ ||
        Form == SeaBirdII::AddressCalc) {
      const bool AtomicRMW =
          Inst.getOpcode() == SeaBird::ATADD ||
          Inst.getOpcode() == SeaBird::ATSUB ||
          Inst.getOpcode() == SeaBird::ATAND ||
          Inst.getOpcode() == SeaBird::ATOR ||
          Inst.getOpcode() == SeaBird::ATXOR ||
          Inst.getOpcode() == SeaBird::SC ||
          Inst.getOpcode() == SeaBird::XCHGmem;
      const bool AtomicCAS = Inst.getOpcode() == SeaBird::CMPXCHG;
      const bool MemoryOnly =
          Inst.getOpcode() == SeaBird::PREFETCH ||
          Inst.getOpcode() == SeaBird::FLUSH ||
          Inst.getOpcode() == SeaBird::INVIC ||
          Inst.getOpcode() == SeaBird::INVDC;
      const bool RegisterFirst = Form != SeaBirdII::StoreQ;
      const unsigned MemoryOp =
          MemoryOnly ? 0
                     : (AtomicCAS ? 3
                                  : (AtomicRMW ? 2
                                               : (RegisterFirst ? 1 : 0)));
      const unsigned RegOp = RegisterFirst ? 0 : 4;
      const unsigned Reg =
          MemoryOnly ? 0 : registerNumber(Inst.getOperand(RegOp).getReg());
      const unsigned Base = registerNumber(Inst.getOperand(MemoryOp).getReg());
      const MCRegister IndexReg = Inst.getOperand(MemoryOp + 1).getReg();
      const bool HasIndex = IndexReg != SeaBird::R4 &&
                            IndexReg != SeaBird::NOIDX;
      const unsigned Index = HasIndex ? registerNumber(IndexReg) : 4;
      const std::int64_t Scale = Inst.getOperand(MemoryOp + 2).getImm();
      const std::int64_t Disp = Inst.getOperand(MemoryOp + 3).getImm();
      const bool SIB = HasIndex || (Base & 7) == 4;
      unsigned Mod = 0;
      if (Disp != 0 || (!SIB && (Base & 7) == 5) ||
          (SIB && (Base & 7) == 5))
        Mod = isInt<8>(Disp) ? 1 : 2;
      if (!isInt<32>(Disp)) {
        Context.reportError(Inst.getLoc(), "SeaBird memory displacement out of range");
        return;
      }
      const bool Extended =
          (!MemoryOnly && Reg > 7) || Base > 7 || (HasIndex && Index > 7);
      if (Extended) {
        emitByte(Code, 0xFE);
        emitByte(Code, 0x80);
      }
      emitByte(Code, Opcode);
      emitByte(Code, static_cast<std::uint8_t>((Mod << 6) | ((Reg & 7) << 3) |
                                               (SIB ? 4 : (Base & 7))));
      if (SIB) {
        const unsigned ScaleBits = Scale == 1 ? 0 : Scale == 2 ? 1 :
                                   Scale == 4 ? 2 : 3;
        emitByte(Code, static_cast<std::uint8_t>((ScaleBits << 6) |
                                                 ((Index & 7) << 3) |
                                                 (Base & 7)));
      }
      if (Extended)
        emitByte(Code, static_cast<std::uint8_t>(
                           ((Reg >> 3) & 3) |
                           (SIB ? (((Index >> 3) & 3) << 4) |
                                      (((Base >> 3) & 3) << 6)
                                : (((Base >> 3) & 3) << 2))));
      if (Inst.getOpcode() == SeaBird::STP)
        emitByte(Code, static_cast<std::uint8_t>(
                           registerNumber(Inst.getOperand(5).getReg())));
      if (AtomicCAS)
        emitByte(Code, static_cast<std::uint8_t>(
                           registerNumber(Inst.getOperand(2).getReg())));
      if (Mod == 1)
        emitByte(Code, static_cast<std::uint8_t>(Disp));
      else if (Mod == 2)
        emitLittleEndian(Code, static_cast<std::uint32_t>(Disp), 4);
      return;
    }

    assert((Form == SeaBirdII::CopyRR || Form == SeaBirdII::ALURR ||
            Form == SeaBirdII::CompareRR || Form == SeaBirdII::UnaryRR) &&
           "unknown SeaBird encoding form");
    const unsigned Dst = registerNumber(Inst.getOperand(0).getReg());
    const unsigned Src =
        registerNumber(Inst.getOperand(Inst.getNumOperands() - 1).getReg());
    const bool Extended = Dst > 7 || Src > 7;
    if (Extended) {
      emitByte(Code, 0xFE);
      emitByte(Code, 0x80);
    }
    emitByte(Code, Opcode);
    emitByte(Code, static_cast<std::uint8_t>(0xC0 | ((Dst & 7) << 3) |
                                             (Src & 7)));
    if (Extended)
      emitByte(Code, static_cast<std::uint8_t>(((Dst >> 3) & 3) |
                                               (((Src >> 3) & 3) << 2)));
  }
};

} // namespace

MCCodeEmitter *llvm::createSeaBirdMCCodeEmitter(const MCInstrInfo &MCII,
                                                 MCContext &Context) {
  return new SeaBirdMCCodeEmitter(MCII, Context);
}
