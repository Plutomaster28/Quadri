#include "SeaBirdISelLowering.h"
#include "SeaBirdMachineFunctionInfo.h"
#include "SeaBirdSubtarget.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#include "SeaBirdGenCallingConv.inc"

SeaBirdTargetLowering::SeaBirdTargetLowering(const TargetMachine &TM,
                                             const SeaBirdSubtarget &STI)
    : TargetLowering(TM, STI), Is64Bit(STI.is64Bit()) {
  if (Is64Bit) {
    addRegisterClass(MVT::i64, &SeaBird::GPR64RegClass);
    addRegisterClass(MVT::f32, &SeaBird::FPR32RegClass);
    addRegisterClass(MVT::f64, &SeaBird::FPR64RegClass);
    addRegisterClass(MVT::f128, &SeaBird::FPR128RegClass);
    addRegisterClass(MVT::v2i64, &SeaBird::VR128RegClass);
    setOperationAction(ISD::FMA, MVT::f64, Legal);
    setOperationAction(ISD::FMA, MVT::f32, Legal);
    setOperationAction(ISD::FMA, MVT::f128, Legal);
    setOperationAction(ISD::FMINNUM, MVT::f32, Legal);
    setOperationAction(ISD::FMAXNUM, MVT::f32, Legal);
    setOperationAction(ISD::FMINNUM, MVT::f64, Legal);
    setOperationAction(ISD::FMAXNUM, MVT::f64, Legal);
    setOperationAction(ISD::FMINNUM, MVT::f128, Legal);
    setOperationAction(ISD::FMAXNUM, MVT::f128, Legal);
    setOperationAction(ISD::SETCC, MVT::f32, Custom);
    setOperationAction(ISD::SETCC, MVT::f64, Custom);
    setOperationAction(ISD::SETCC, MVT::f128, Custom);
    setOperationAction(ISD::BR_CC, MVT::f32, Expand);
    setOperationAction(ISD::BR_CC, MVT::f64, Expand);
    setOperationAction(ISD::BR_CC, MVT::f128, Expand);
    setOperationAction(ISD::SELECT, MVT::f32, Legal);
    setOperationAction(ISD::SELECT, MVT::f64, Legal);
    setOperationAction(ISD::SELECT, MVT::f128, Legal);
    setOperationAction(ISD::SELECT_CC, MVT::f32, Expand);
    setOperationAction(ISD::SELECT_CC, MVT::f64, Expand);
    setOperationAction(ISD::SELECT_CC, MVT::f128, Expand);
    setOperationAction(ISD::ABS, MVT::i64, Legal);
    setOperationAction(ISD::CTLZ, MVT::i64, Legal);
    setOperationAction(ISD::CTTZ, MVT::i64, Legal);
    setOperationAction(ISD::CTPOP, MVT::i64, Legal);
    setOperationAction(ISD::UREM, MVT::i64, Expand);
    setOperationAction(ISD::UDIVREM, MVT::i64, Expand);
    setOperationAction(ISD::SDIVREM, MVT::i64, Expand);
    setOperationAction(ISD::ATOMIC_SWAP, MVT::i64, Legal);
    setOperationAction(ISD::UINT_TO_FP, MVT::i64, Custom);
    setOperationAction(ISD::FP_TO_UINT, MVT::i64, Custom);
    setOperationAction(ISD::UDIV, MVT::v2i64, Legal);
    setOperationAction(ISD::ABS, MVT::v2i64, Legal);
    setOperationAction(ISD::SMAX, MVT::v2i64, Legal);
    setOperationAction(ISD::SMIN, MVT::v2i64, Legal);
  } else {
    addRegisterClass(MVT::i32, &SeaBird::GPR32RegClass);
  }
  setMinFunctionAlignment(Align(1));
  setPrefFunctionAlignment(Align(1));
  setStackPointerRegisterToSaveRestore(SeaBird::R7);
  setMinimumJumpTableEntries(UINT_MAX);
  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent);
  computeRegisterProperties(STI.getRegisterInfo());
  const MVT ScalarVT = Is64Bit ? MVT::i64 : MVT::i32;
  setOperationAction(ISD::BR_CC, ScalarVT, Custom);
  setOperationAction(ISD::SETCC, ScalarVT, Custom);
  setOperationAction(ISD::SELECT, ScalarVT, Custom);
  setOperationAction(ISD::SELECT_CC, ScalarVT, Custom);
  setOperationAction(ISD::SIGN_EXTEND_INREG, MVT::i1, Expand);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, ScalarVT, Expand);
  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::VAARG, MVT::Other, Custom);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);
  if (!Is64Bit) {
    setOperationAction(ISD::BRCOND, MVT::Other, Custom);
    setOperationAction(ISD::SHL_PARTS, MVT::i32, Custom);
    setOperationAction(ISD::SRL_PARTS, MVT::i32, Custom);
    setOperationAction(ISD::SRA_PARTS, MVT::i32, Custom);
    setOperationAction(ISD::UMUL_LOHI, MVT::i32, Custom);
    setOperationAction(ISD::SADDSAT, MVT::i32, Legal);
    setOperationAction(ISD::UADDSAT, MVT::i32, Legal);
    setOperationAction(ISD::SSUBSAT, MVT::i32, Legal);
    setOperationAction(ISD::USUBSAT, MVT::i32, Legal);
    setOperationAction(ISD::SMAX, MVT::i32, Legal);
    setOperationAction(ISD::SMIN, MVT::i32, Legal);
    setOperationAction(ISD::ABS, MVT::i32, Legal);
    setOperationAction(ISD::CTLZ, MVT::i32, Legal);
    setOperationAction(ISD::CTTZ, MVT::i32, Legal);
    setOperationAction(ISD::CTPOP, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_LOAD, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_STORE, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_LOAD_ADD, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_LOAD_SUB, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_LOAD_AND, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_LOAD_OR, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_LOAD_XOR, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_CMP_SWAP, MVT::i32, Legal);
    setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Legal);
    setLibcallImpl(RTLIB::SDIV_I64, RTLIB::impl___divdi3);
    setLibcallImpl(RTLIB::UDIV_I64, RTLIB::impl___udivdi3);
    setLibcallImpl(RTLIB::SREM_I64, RTLIB::impl___moddi3);
    setLibcallImpl(RTLIB::UREM_I64, RTLIB::impl___umoddi3);
  }
}

const char *SeaBirdTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch (Opcode) {
  case SeaBirdISD::RET_GLUE:
    return "SeaBirdISD::RET_GLUE";
  case SeaBirdISD::CALL:
    return "SeaBirdISD::CALL";
  case SeaBirdISD::CMP:
    return "SeaBirdISD::CMP";
  case SeaBirdISD::SETLT:
    return "SeaBirdISD::SETLT";
  case SeaBirdISD::MULH:
    return "SeaBirdISD::MULH";
  case SeaBirdISD::FPSETCC:
    return "SeaBirdISD::FPSETCC";
  case SeaBirdISD::BR_EQ:
    return "SeaBirdISD::BR_EQ";
  case SeaBirdISD::BR_NE:
    return "SeaBirdISD::BR_NE";
  case SeaBirdISD::BR_GT: return "SeaBirdISD::BR_GT";
  case SeaBirdISD::BR_GE: return "SeaBirdISD::BR_GE";
  case SeaBirdISD::BR_LT: return "SeaBirdISD::BR_LT";
  case SeaBirdISD::BR_LE: return "SeaBirdISD::BR_LE";
  case SeaBirdISD::BR_C: return "SeaBirdISD::BR_C";
  case SeaBirdISD::BR_NC: return "SeaBirdISD::BR_NC";
  default:
    return nullptr;
  }
}

SDValue SeaBirdTargetLowering::LowerOperation(SDValue Op,
                                               SelectionDAG &DAG) const {
  auto MaterializeCC = [&](SDValue LHS, SDValue RHS, ISD::CondCode CC,
                           const SDLoc &DL) -> SDValue {
    const EVT VT = LHS.getValueType();
    if (VT.isFloatingPoint())
      return DAG.getNode(
          SeaBirdISD::FPSETCC, DL, MVT::i64, LHS, RHS,
          DAG.getConstant(static_cast<unsigned>(CC), DL, MVT::i64));
    const SDValue One = DAG.getConstant(1, DL, VT);

    auto SignedLT = [&](SDValue A, SDValue B) {
      return DAG.getNode(SeaBirdISD::SETLT, DL, VT, A, B);
    };
    auto UnsignedLT = [&](SDValue A, SDValue B) {
      const std::uint64_t SignBit =
          VT == MVT::i64 ? (std::uint64_t(1) << 63) : 0x80000000ULL;
      SDValue Sign = DAG.getConstant(SignBit, DL, VT);
      A = DAG.getNode(ISD::XOR, DL, VT, A, Sign);
      B = DAG.getNode(ISD::XOR, DL, VT, B, Sign);
      return SignedLT(A, B);
    };
    auto Invert = [&](SDValue Value) {
      return DAG.getNode(ISD::XOR, DL, VT, Value, One);
    };

    switch (CC) {
    case ISD::SETLT:
    case ISD::SETOLT:
      return SignedLT(LHS, RHS);
    case ISD::SETGT:
    case ISD::SETOGT:
      return SignedLT(RHS, LHS);
    case ISD::SETGE:
    case ISD::SETOGE:
      return Invert(SignedLT(LHS, RHS));
    case ISD::SETLE:
    case ISD::SETOLE:
      return Invert(SignedLT(RHS, LHS));
    case ISD::SETULT:
      return UnsignedLT(LHS, RHS);
    case ISD::SETUGT:
      return UnsignedLT(RHS, LHS);
    case ISD::SETUGE:
      return Invert(UnsignedLT(LHS, RHS));
    case ISD::SETULE:
      return Invert(UnsignedLT(RHS, LHS));
    case ISD::SETEQ:
    case ISD::SETOEQ: {
      SDValue EitherLT = DAG.getNode(ISD::OR, DL, VT,
                                     SignedLT(LHS, RHS), SignedLT(RHS, LHS));
      return Invert(EitherLT);
    }
    case ISD::SETNE:
    case ISD::SETONE:
      return DAG.getNode(ISD::OR, DL, VT, SignedLT(LHS, RHS),
                         SignedLT(RHS, LHS));
    default:
      report_fatal_error("unsupported SeaBird set condition");
    }
  };

  if (Op.getOpcode() == ISD::UMUL_LOHI) {
    SDLoc DL(Op);
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    SDValue Shift = DAG.getConstant(31, DL, MVT::i32);
    SDValue Low = DAG.getNode(ISD::MUL, DL, MVT::i32, LHS, RHS);
    SDValue SignedHigh =
        DAG.getNode(SeaBirdISD::MULH, DL, MVT::i32, LHS, RHS);
    SDValue LHSSign = DAG.getNode(ISD::SRA, DL, MVT::i32, LHS, Shift);
    SDValue RHSSign = DAG.getNode(ISD::SRA, DL, MVT::i32, RHS, Shift);
    SDValue LHSCorrection =
        DAG.getNode(ISD::AND, DL, MVT::i32, RHS, LHSSign);
    SDValue RHSCorrection =
        DAG.getNode(ISD::AND, DL, MVT::i32, LHS, RHSSign);
    SDValue High = DAG.getNode(
        ISD::ADD, DL, MVT::i32,
        DAG.getNode(ISD::ADD, DL, MVT::i32, SignedHigh, LHSCorrection),
        RHSCorrection);
    return DAG.getMergeValues({Low, High}, DL);
  }

  if (Op.getOpcode() == ISD::UINT_TO_FP) {
    SDLoc DL(Op);
    SDValue Source = Op.getOperand(0);
    const EVT FloatVT = Op.getValueType();
    SDValue Signed = DAG.getNode(ISD::SINT_TO_FP, DL, FloatVT, Source);
    SDValue Half = DAG.getNode(
        ISD::SRL, DL, MVT::i64, Source,
        DAG.getConstant(1, DL, MVT::i64));
    SDValue LowBit = DAG.getNode(
        ISD::AND, DL, MVT::i64, Source,
        DAG.getConstant(1, DL, MVT::i64));
    SDValue RoundedHalf = DAG.getNode(ISD::OR, DL, MVT::i64, Half, LowBit);
    SDValue Adjusted =
        DAG.getNode(ISD::SINT_TO_FP, DL, FloatVT, RoundedHalf);
    Adjusted = DAG.getNode(ISD::FADD, DL, FloatVT, Adjusted, Adjusted);
    SDValue IsNegative = DAG.getNode(
        SeaBirdISD::SETLT, DL, MVT::i64, Source,
        DAG.getConstant(0, DL, MVT::i64));
    return DAG.getNode(ISD::SELECT, DL, FloatVT, IsNegative, Adjusted,
                       Signed);
  }

  if (Op.getOpcode() == ISD::FP_TO_UINT) {
    SDLoc DL(Op);
    SDValue Source = Op.getOperand(0);
    const EVT FloatVT = Source.getValueType();
    SDValue Threshold = DAG.getConstantFP(9223372036854775808.0, DL, FloatVT);
    SDValue IsSmall = DAG.getSetCC(DL, MVT::i64, Source, Threshold,
                                   ISD::SETOLT);
    SDValue Small = DAG.getNode(ISD::FP_TO_SINT, DL, MVT::i64, Source);
    SDValue Reduced = DAG.getNode(ISD::FSUB, DL, FloatVT, Source, Threshold);
    SDValue Large = DAG.getNode(ISD::FP_TO_SINT, DL, MVT::i64, Reduced);
    Large = DAG.getNode(
        ISD::XOR, DL, MVT::i64, Large,
        DAG.getConstant(std::uint64_t(1) << 63, DL, MVT::i64));
    return DAG.getNode(ISD::SELECT, DL, MVT::i64, IsSmall, Small, Large);
  }

  if (Op.getOpcode() == ISD::SHL_PARTS ||
      Op.getOpcode() == ISD::SRL_PARTS ||
      Op.getOpcode() == ISD::SRA_PARTS) {
    SDLoc DL(Op);
    SDValue Lo = Op.getOperand(0);
    SDValue Hi = Op.getOperand(1);
    SDValue Amount = Op.getOperand(2);
    SDValue Zero = DAG.getConstant(0, DL, MVT::i32);
    SDValue One = DAG.getConstant(1, DL, MVT::i32);
    SDValue LowMask = DAG.getConstant(31, DL, MVT::i32);
    SDValue AllOnes = DAG.getConstant(UINT32_MAX, DL, MVT::i32);

    SDValue K = DAG.getNode(ISD::AND, DL, MVT::i32, Amount, LowMask);
    SDValue NegK = DAG.getNode(ISD::SUB, DL, MVT::i32, Zero, K);
    SDValue LargeBit = DAG.getNode(
        ISD::AND, DL, MVT::i32,
        DAG.getNode(ISD::SRL, DL, MVT::i32, Amount,
                    DAG.getConstant(5, DL, MVT::i32)),
        One);
    SDValue LargeMask =
        DAG.getNode(ISD::SUB, DL, MVT::i32, Zero, LargeBit);
    SDValue SmallMask =
        DAG.getNode(ISD::XOR, DL, MVT::i32, LargeMask, AllOnes);

    SDValue NonZeroBit = DAG.getNode(
        ISD::SRL, DL, MVT::i32,
        DAG.getNode(ISD::OR, DL, MVT::i32, K,
                    DAG.getNode(ISD::SUB, DL, MVT::i32, Zero, K)),
        DAG.getConstant(31, DL, MVT::i32));
    SDValue CrossMask =
        DAG.getNode(ISD::SUB, DL, MVT::i32, Zero, NonZeroBit);

    auto SelectByLarge = [&](SDValue Small, SDValue Large) {
      SDValue SmallPart =
          DAG.getNode(ISD::AND, DL, MVT::i32, Small, SmallMask);
      SDValue LargePart =
          DAG.getNode(ISD::AND, DL, MVT::i32, Large, LargeMask);
      return DAG.getNode(ISD::OR, DL, MVT::i32, SmallPart, LargePart);
    };

    SDValue LoResult;
    SDValue HiResult;
    if (Op.getOpcode() == ISD::SHL_PARTS) {
      SDValue ShiftedLo = DAG.getNode(ISD::SHL, DL, MVT::i32, Lo, K);
      SDValue Cross = DAG.getNode(
          ISD::AND, DL, MVT::i32,
          DAG.getNode(ISD::SRL, DL, MVT::i32, Lo, NegK), CrossMask);
      SDValue SmallHi = DAG.getNode(
          ISD::OR, DL, MVT::i32,
          DAG.getNode(ISD::SHL, DL, MVT::i32, Hi, K), Cross);
      LoResult =
          DAG.getNode(ISD::AND, DL, MVT::i32, ShiftedLo, SmallMask);
      HiResult = SelectByLarge(SmallHi, ShiftedLo);
    } else {
      const unsigned HiShift =
          Op.getOpcode() == ISD::SRA_PARTS ? ISD::SRA : ISD::SRL;
      SDValue ShiftedHi =
          DAG.getNode(HiShift, DL, MVT::i32, Hi, K);
      SDValue Cross = DAG.getNode(
          ISD::AND, DL, MVT::i32,
          DAG.getNode(ISD::SHL, DL, MVT::i32, Hi, NegK), CrossMask);
      SDValue SmallLo = DAG.getNode(
          ISD::OR, DL, MVT::i32,
          DAG.getNode(ISD::SRL, DL, MVT::i32, Lo, K), Cross);
      LoResult = SelectByLarge(SmallLo, ShiftedHi);
      SDValue LargeHi =
          Op.getOpcode() == ISD::SRA_PARTS
              ? DAG.getNode(ISD::SRA, DL, MVT::i32, Hi,
                            DAG.getConstant(31, DL, MVT::i32))
              : Zero;
      HiResult = SelectByLarge(ShiftedHi, LargeHi);
    }
    return DAG.getMergeValues({LoResult, HiResult}, DL);
  }

  if (Op.getOpcode() == ISD::SETCC) {
    SDLoc DL(Op);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
    return MaterializeCC(Op.getOperand(0), Op.getOperand(1), CC, DL);
  }

  if (Op.getOpcode() == ISD::SELECT_CC) {
    SDLoc DL(Op);
    const EVT VT = Op.getValueType();
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
    SDValue Condition =
        MaterializeCC(Op.getOperand(0), Op.getOperand(1), CC, DL);
    SDValue Zero = DAG.getConstant(0, DL, VT);
    SDValue Mask = DAG.getNode(ISD::SUB, DL, VT, Zero, Condition);
    SDValue InverseMask =
        DAG.getNode(ISD::XOR, DL, VT, Mask,
                    DAG.getConstant(VT == MVT::i64 ? UINT64_MAX : UINT32_MAX,
                                    DL, VT));
    SDValue TruePart =
        DAG.getNode(ISD::AND, DL, VT, Op.getOperand(2), Mask);
    SDValue FalsePart =
        DAG.getNode(ISD::AND, DL, VT, Op.getOperand(3), InverseMask);
    return DAG.getNode(ISD::OR, DL, VT, TruePart, FalsePart);
  }

  if (Op.getOpcode() == ISD::SELECT) {
    SDLoc DL(Op);
    const EVT VT = Op.getValueType();
    SDValue Zero = DAG.getConstant(0, DL, VT);
    SDValue Mask =
        DAG.getNode(ISD::SUB, DL, VT, Zero, Op.getOperand(0));
    SDValue InverseMask =
        DAG.getNode(ISD::XOR, DL, VT, Mask,
                    DAG.getConstant(VT == MVT::i64 ? UINT64_MAX : UINT32_MAX,
                                    DL, VT));
    SDValue TruePart =
        DAG.getNode(ISD::AND, DL, VT, Op.getOperand(1), Mask);
    SDValue FalsePart =
        DAG.getNode(ISD::AND, DL, VT, Op.getOperand(2), InverseMask);
    return DAG.getNode(ISD::OR, DL, VT, TruePart, FalsePart);
  }

  if (Op.getOpcode() == ISD::BRCOND) {
    SDLoc DL(Op);
    SDValue Chain = Op.getOperand(0);
    SDValue Condition = Op.getOperand(1);
    SDValue Destination = Op.getOperand(2);
    SDValue Compare = DAG.getNode(
        SeaBirdISD::CMP, DL, DAG.getVTList(MVT::Other, MVT::Glue), Chain,
        Condition, DAG.getConstant(0, DL, MVT::i32));
    return DAG.getNode(SeaBirdISD::BR_NE, DL, MVT::Other, Compare,
                       Destination, Compare.getValue(1));
  }

  if (Op.getOpcode() == ISD::VASTART)
    return LowerVASTART(Op, DAG);
  if (Op.getOpcode() == ISD::VAARG)
    return LowerVAARG(Op, DAG);

  if (Op.getOpcode() != ISD::BR_CC)
    llvm_unreachable("unexpected custom SeaBird lowering opcode");

  SDLoc DL(Op);
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Destination = Op.getOperand(4);
  bool Swap = CC == ISD::SETUGT || CC == ISD::SETULE;
  if (Swap)
    std::swap(LHS, RHS);

  SDValue Compare = DAG.getNode(SeaBirdISD::CMP, DL,
                                DAG.getVTList(MVT::Other, MVT::Glue),
                                Chain, LHS, RHS);
  unsigned BranchOpcode;
  switch (CC) {
  case ISD::SETEQ: BranchOpcode = SeaBirdISD::BR_EQ; break;
  case ISD::SETNE: BranchOpcode = SeaBirdISD::BR_NE; break;
  case ISD::SETGT: BranchOpcode = SeaBirdISD::BR_GT; break;
  case ISD::SETGE: BranchOpcode = SeaBirdISD::BR_GE; break;
  case ISD::SETLT: BranchOpcode = SeaBirdISD::BR_LT; break;
  case ISD::SETLE: BranchOpcode = SeaBirdISD::BR_LE; break;
  case ISD::SETULT:
  case ISD::SETUGT: BranchOpcode = SeaBirdISD::BR_C; break;
  case ISD::SETUGE:
  case ISD::SETULE: BranchOpcode = SeaBirdISD::BR_NC; break;
  default: report_fatal_error("unsupported SeaBird integer branch condition");
  }
  return DAG.getNode(BranchOpcode, DL, MVT::Other, Compare, Destination,
                     Compare.getValue(1));
}

MachineBasicBlock *SeaBirdTargetLowering::EmitInstrWithCustomInserter(
    MachineInstr &MI, MachineBasicBlock *MBB) const {
  if (MI.getOpcode() == SeaBird::FPSELECT32 ||
      MI.getOpcode() == SeaBird::FPSELECT64 ||
      MI.getOpcode() == SeaBird::FPSELECT128) {
    MachineFunction *MF = MBB->getParent();
    const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();
    const DebugLoc DL = MI.getDebugLoc();
    const BasicBlock *IRBB = MBB->getBasicBlock();
    MachineFunction::iterator InsertAt = std::next(MBB->getIterator());
    MachineBasicBlock *FalseMBB = MF->CreateMachineBasicBlock(IRBB);
    MachineBasicBlock *SinkMBB = MF->CreateMachineBasicBlock(IRBB);
    MF->insert(InsertAt, FalseMBB);
    MF->insert(InsertAt, SinkMBB);

    SinkMBB->splice(SinkMBB->begin(), MBB,
                    std::next(MachineBasicBlock::iterator(MI)), MBB->end());
    SinkMBB->transferSuccessorsAndUpdatePHIs(MBB);
    BuildMI(*MBB, MI, DL, TII.get(SeaBird::JNZR))
        .addReg(MI.getOperand(1).getReg())
        .addMBB(SinkMBB);
    BuildMI(*MBB, MI, DL, TII.get(SeaBird::JMP)).addMBB(FalseMBB);
    MBB->addSuccessor(SinkMBB);
    MBB->addSuccessor(FalseMBB);
    BuildMI(*FalseMBB, FalseMBB->end(), DL, TII.get(SeaBird::JMP))
        .addMBB(SinkMBB);
    FalseMBB->addSuccessor(SinkMBB);
    BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(TargetOpcode::PHI),
            MI.getOperand(0).getReg())
        .addReg(MI.getOperand(2).getReg())
        .addMBB(MBB)
        .addReg(MI.getOperand(3).getReg())
        .addMBB(FalseMBB);
    MI.eraseFromParent();
    return SinkMBB;
  }

  assert((MI.getOpcode() == SeaBird::FPSETCC32 ||
          MI.getOpcode() == SeaBird::FPSETCC64 ||
          MI.getOpcode() == SeaBird::FPSETCC128) &&
         "unexpected SeaBird custom-inserter pseudo");

  MachineFunction *MF = MBB->getParent();
  const TargetInstrInfo &TII = *MF->getSubtarget().getInstrInfo();
  MachineRegisterInfo &MRI = MF->getRegInfo();
  const DebugLoc DL = MI.getDebugLoc();
  const BasicBlock *IRBB = MBB->getBasicBlock();
  MachineFunction::iterator InsertAt = std::next(MBB->getIterator());

  MachineBasicBlock *ZClearMBB = MF->CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *ZSetMBB = MF->CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *TrueMBB = MF->CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *FalseMBB = MF->CreateMachineBasicBlock(IRBB);
  MachineBasicBlock *SinkMBB = MF->CreateMachineBasicBlock(IRBB);
  MF->insert(InsertAt, ZClearMBB);
  MF->insert(InsertAt, ZSetMBB);
  MF->insert(InsertAt, TrueMBB);
  MF->insert(InsertAt, FalseMBB);
  MF->insert(InsertAt, SinkMBB);

  SinkMBB->splice(SinkMBB->begin(), MBB,
                  std::next(MachineBasicBlock::iterator(MI)), MBB->end());
  SinkMBB->transferSuccessorsAndUpdatePHIs(MBB);

  const unsigned CC = static_cast<unsigned>(MI.getOperand(3).getImm());
  auto CategoryTarget = [&](unsigned Bit) {
    return (CC & Bit) ? TrueMBB : FalseMBB;
  };
  MachineBasicBlock *EqualTarget = CategoryTarget(1);
  MachineBasicBlock *GreaterTarget = CategoryTarget(2);
  MachineBasicBlock *LessTarget = CategoryTarget(4);
  MachineBasicBlock *UnorderedTarget = CategoryTarget(8);

  const unsigned CompareOpcode = MI.getOpcode() == SeaBird::FPSETCC32
                                     ? SeaBird::FCMP32CG
                                 : MI.getOpcode() == SeaBird::FPSETCC128
                                     ? SeaBird::FCMP128CG
                                     : SeaBird::FCMP64;
  BuildMI(*MBB, MI, DL, TII.get(CompareOpcode))
      .addReg(MI.getOperand(1).getReg())
      .addReg(MI.getOperand(2).getReg());
  BuildMI(*MBB, MI, DL, TII.get(SeaBird::JE)).addMBB(ZSetMBB);
  BuildMI(*MBB, MI, DL, TII.get(SeaBird::JMP)).addMBB(ZClearMBB);
  MBB->addSuccessor(ZSetMBB);
  MBB->addSuccessor(ZClearMBB);

  auto EmitDecision = [&](MachineBasicBlock *DecisionMBB,
                          MachineBasicBlock *CarryTarget,
                          MachineBasicBlock *NoCarryTarget) {
    if (CarryTarget == NoCarryTarget) {
      BuildMI(*DecisionMBB, DecisionMBB->end(), DL,
              TII.get(SeaBird::JMP)).addMBB(CarryTarget);
      DecisionMBB->addSuccessor(CarryTarget);
      return;
    }
    BuildMI(*DecisionMBB, DecisionMBB->end(), DL,
            TII.get(SeaBird::JC)).addMBB(CarryTarget);
    BuildMI(*DecisionMBB, DecisionMBB->end(), DL,
            TII.get(SeaBird::JMP)).addMBB(NoCarryTarget);
    DecisionMBB->addSuccessor(CarryTarget);
    DecisionMBB->addSuccessor(NoCarryTarget);
  };
  EmitDecision(ZClearMBB, LessTarget, GreaterTarget);
  EmitDecision(ZSetMBB, UnorderedTarget, EqualTarget);

  Register TrueReg = MRI.createVirtualRegister(&SeaBird::GPR64RegClass);
  Register FalseReg = MRI.createVirtualRegister(&SeaBird::GPR64RegClass);
  BuildMI(*TrueMBB, TrueMBB->end(), DL, TII.get(SeaBird::MOVI64), TrueReg)
      .addImm(1);
  BuildMI(*TrueMBB, TrueMBB->end(), DL, TII.get(SeaBird::JMP))
      .addMBB(SinkMBB);
  TrueMBB->addSuccessor(SinkMBB);
  BuildMI(*FalseMBB, FalseMBB->end(), DL, TII.get(SeaBird::MOVI64), FalseReg)
      .addImm(0);
  BuildMI(*FalseMBB, FalseMBB->end(), DL, TII.get(SeaBird::JMP))
      .addMBB(SinkMBB);
  FalseMBB->addSuccessor(SinkMBB);

  BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(TargetOpcode::PHI),
          MI.getOperand(0).getReg())
      .addReg(TrueReg)
      .addMBB(TrueMBB)
      .addReg(FalseReg)
      .addMBB(FalseMBB);
  MI.eraseFromParent();
  return SinkMBB;
}

SDValue SeaBirdTargetLowering::LowerVASTART(SDValue Op,
                                             SelectionDAG &DAG) const {
  MachineFunction &MF = DAG.getMachineFunction();
  auto *FuncInfo = MF.getInfo<SeaBirdMachineFunctionInfo>();
  SDLoc DL(Op);
  SDValue FirstVarArg =
      DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(),
                        getPointerTy(MF.getDataLayout()));
  const Value *SourceValue =
      cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  return DAG.getStore(Op.getOperand(0), DL, FirstVarArg, Op.getOperand(1),
                      MachinePointerInfo(SourceValue));
}

SDValue SeaBirdTargetLowering::LowerVAARG(SDValue Op,
                                           SelectionDAG &DAG) const {
  SDNode *Node = Op.getNode();
  const EVT VT = Node->getValueType(0);
  SDValue Chain = Node->getOperand(0);
  SDValue VAListAddress = Node->getOperand(1);
  const Value *SourceValue =
      cast<SrcValueSDNode>(Node->getOperand(2))->getValue();
  const Align ArgumentAlign =
      MaybeAlign(Node->getConstantOperandVal(3)).valueOrOne();
  const unsigned SlotBytes = Is64Bit ? 8 : 4;
  const unsigned EffectiveAlign =
      std::max<unsigned>(SlotBytes, ArgumentAlign.value());
  const unsigned ArgumentBytes =
      DAG.getDataLayout()
          .getTypeAllocSize(VT.getTypeForEVT(*DAG.getContext()))
          .getFixedValue();
  const unsigned Advance = alignTo(ArgumentBytes, SlotBytes);
  SDLoc DL(Op);
  const EVT PointerVT = getPointerTy(DAG.getDataLayout());

  SDValue PointerLoad =
      DAG.getLoad(PointerVT, DL, Chain, VAListAddress,
                  MachinePointerInfo(SourceValue));
  SDValue ArgumentAddress = PointerLoad;
  if (EffectiveAlign > SlotBytes) {
    ArgumentAddress = DAG.getNode(
        ISD::ADD, DL, PointerVT, ArgumentAddress,
        DAG.getConstant(EffectiveAlign - 1, DL, PointerVT));
    ArgumentAddress = DAG.getNode(
        ISD::AND, DL, PointerVT, ArgumentAddress,
        DAG.getSignedConstant(-static_cast<int64_t>(EffectiveAlign), DL,
                              PointerVT));
  }
  SDValue NextAddress =
      DAG.getNode(ISD::ADD, DL, PointerVT, ArgumentAddress,
                  DAG.getConstant(Advance, DL, PointerVT));
  Chain = DAG.getStore(PointerLoad.getValue(1), DL, NextAddress,
                       VAListAddress, MachinePointerInfo(SourceValue));
  return DAG.getLoad(VT, DL, Chain, ArgumentAddress, MachinePointerInfo());
}

SDValue SeaBirdTargetLowering::LowerCall(
    CallLoweringInfo &CLI, SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &DL = CLI.DL;
  CLI.IsTailCall = false;
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CLI.CallConv, CLI.IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeCallOperands(CLI.Outs,
                             Is64Bit ? CC_SeaBird : CC_SeaBird32);

  const unsigned StackBytes = alignTo(CCInfo.getStackSize(), 16u);
  SDValue Chain = DAG.getCALLSEQ_START(CLI.Chain, StackBytes, 0, DL);
  SDValue Glue;
  SmallVector<std::pair<MCRegister, SDValue>, 10> RegsToPass;
  SmallVector<SDValue, 4> MemChains;
  for (unsigned I = 0; I < ArgLocs.size(); ++I) {
    const CCValAssign &VA = ArgLocs[I];
    SDValue Arg = CLI.OutVals[I];
    switch (VA.getLocInfo()) {
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Arg);
      break;
    default:
      llvm_unreachable("unsupported SeaBird call argument promotion");
    }
    if (VA.isRegLoc()) {
      RegsToPass.emplace_back(VA.getLocReg(), Arg);
    } else {
      EVT PtrVT = getPointerTy(DAG.getDataLayout());
      SDValue SP = DAG.getRegister(SeaBird::R7, PtrVT);
      SDValue Address = DAG.getNode(
          ISD::ADD, DL, PtrVT, SP,
          DAG.getConstant(VA.getLocMemOffset(), DL, PtrVT));
      MemChains.push_back(DAG.getStore(Chain, DL, Arg, Address,
                                      MachinePointerInfo()));
    }
  }

  if (!MemChains.empty())
    Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemChains);

  for (const auto &[Reg, Value] : RegsToPass) {
    Chain = DAG.getCopyToReg(Chain, DL, Reg, Value, Glue);
    Glue = Chain.getValue(1);
  }

  SDValue Callee = CLI.Callee;
  EVT PtrVT = getPointerTy(DAG.getDataLayout());
  if (auto *Global = dyn_cast<GlobalAddressSDNode>(Callee))
    Callee = DAG.getTargetGlobalAddress(Global->getGlobal(), DL, PtrVT);
  else if (auto *External = dyn_cast<ExternalSymbolSDNode>(Callee))
    Callee = DAG.getTargetExternalSymbol(External->getSymbol(), PtrVT);
  else if (Callee.getValueType() != PtrVT)
    Callee = DAG.getNode(ISD::BITCAST, DL, PtrVT, Callee);

  SmallVector<SDValue, 16> Ops{Chain, Callee};
  const std::uint32_t *Mask =
      DAG.getMachineFunction().getSubtarget().getRegisterInfo()->
          getCallPreservedMask(DAG.getMachineFunction(), CLI.CallConv);
  Ops.push_back(DAG.getRegisterMask(Mask));
  for (const auto &[Reg, Value] : RegsToPass)
    Ops.push_back(DAG.getRegister(Reg, Value.getValueType()));
  if (Glue)
    Ops.push_back(Glue);

  Chain = DAG.getNode(SeaBirdISD::CALL, DL,
                      DAG.getVTList(MVT::Other, MVT::Glue), Ops);
  Glue = Chain.getValue(1);
  Chain = DAG.getCALLSEQ_END(Chain, StackBytes, 0, Glue, DL);
  Glue = Chain.getValue(1);
  return LowerCallResult(Chain, Glue, CLI.CallConv, CLI.IsVarArg, CLI.Ins, DL,
                         DAG, InVals);
}

SDValue SeaBirdTargetLowering::LowerCallResult(
    SDValue Chain, SDValue Glue, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  SmallVector<CCValAssign, 4> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeCallResult(Ins,
                           Is64Bit ? RetCC_SeaBird : RetCC_SeaBird32);
  for (const CCValAssign &VA : RVLocs) {
    SDValue Copy = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getValVT(),
                                      Glue);
    InVals.push_back(Copy);
    Chain = Copy.getValue(1);
    Glue = Copy.getValue(2);
  }
  return Chain;
}

SDValue SeaBirdTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), ArgLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeFormalArguments(Ins,
                                Is64Bit ? CC_SeaBird : CC_SeaBird32);

  if (IsVarArg) {
    MachineFunction &MF = DAG.getMachineFunction();
    MachineFrameInfo &MFI = MF.getFrameInfo();
    const unsigned SlotBytes = Is64Bit ? 8 : 4;
    const int FirstVarArgOffset = CCInfo.getStackSize() + SlotBytes;
    int FI = MFI.CreateFixedObject(SlotBytes, FirstVarArgOffset, true);
    MF.getInfo<SeaBirdMachineFunctionInfo>()->setVarArgsFrameIndex(FI);
  }

  MachineRegisterInfo &MRI = DAG.getMachineFunction().getRegInfo();
  for (const CCValAssign &VA : ArgLocs) {
    if (VA.isMemLoc()) {
      MachineFrameInfo &MFI = DAG.getMachineFunction().getFrameInfo();
      const unsigned SlotBytes = Is64Bit ? 8 : 4;
      const unsigned ArgumentBytes = VA.getLocVT().getStoreSize();
      const MVT PtrVT = Is64Bit ? MVT::i64 : MVT::i32;
      int FI = MFI.CreateFixedObject(ArgumentBytes,
                                     VA.getLocMemOffset() + SlotBytes, true);
      SDValue Address = DAG.getFrameIndex(FI, PtrVT);
      SDValue Load = DAG.getLoad(VA.getLocVT(), DL, Chain, Address,
                                 MachinePointerInfo::getFixedStack(
                                     DAG.getMachineFunction(), FI));
      InVals.push_back(Load);
      Chain = Load.getValue(1);
      continue;
    }
    const TargetRegisterClass *RC = getRegClassFor(VA.getLocVT());
    Register VReg = MRI.createVirtualRegister(RC);
    MRI.addLiveIn(VA.getLocReg(), VReg);
    SDValue Value = DAG.getCopyFromReg(Chain, DL, VReg, VA.getLocVT());
    if (VA.getLocInfo() == CCValAssign::SExt)
      Value = DAG.getNode(ISD::AssertSext, DL, VA.getLocVT(), Value,
                          DAG.getValueType(VA.getValVT()));
    else if (VA.getLocInfo() == CCValAssign::ZExt)
      Value = DAG.getNode(ISD::AssertZext, DL, VA.getLocVT(), Value,
                          DAG.getValueType(VA.getValVT()));
    if (VA.getLocInfo() != CCValAssign::Full)
      Value = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Value);
    InVals.push_back(Value);
  }
  return Chain;
}

bool SeaBirdTargetLowering::CanLowerReturn(
    CallingConv::ID CallConv, MachineFunction &MF, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs, LLVMContext &Context,
    const Type *RetTy) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
  return CCInfo.CheckReturn(Outs,
                            Is64Bit ? RetCC_SeaBird : RetCC_SeaBird32);
}

SDValue SeaBirdTargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, IsVarArg, DAG.getMachineFunction(), RVLocs,
                 *DAG.getContext());
  CCInfo.AnalyzeReturn(Outs,
                       Is64Bit ? RetCC_SeaBird : RetCC_SeaBird32);

  SDValue Glue;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  for (unsigned I = 0; I < RVLocs.size(); ++I) {
    const CCValAssign &VA = RVLocs[I];
    Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), OutVals[I], Glue);
    Glue = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }
  RetOps[0] = Chain;
  if (Glue)
    RetOps.push_back(Glue);
  return DAG.getNode(SeaBirdISD::RET_GLUE, DL, MVT::Other, RetOps);
}
