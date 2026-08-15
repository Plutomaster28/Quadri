#ifndef LLVM_LIB_TARGET_SEABIRD_SEABIRDISELLOWERING_H
#define LLVM_LIB_TARGET_SEABIRD_SEABIRDISELLOWERING_H

#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {

class SeaBirdSubtarget;

namespace SeaBirdISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  RET_GLUE,
  CALL,
  CMP,
  SETLT,
  MULH,
  FPSETCC,
  BR_EQ,
  BR_NE,
  BR_GT,
  BR_GE,
  BR_LT,
  BR_LE,
  BR_C,
  BR_NC
};
}

class SeaBirdTargetLowering final : public TargetLowering {
  bool Is64Bit;

public:
  SeaBirdTargetLowering(const TargetMachine &TM, const SeaBirdSubtarget &STI);

  const char *getTargetNodeName(unsigned Opcode) const override;
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *MBB) const override;
  bool isCheapToSpeculateCtlz(Type *) const override { return !Is64Bit; }
  bool isCheapToSpeculateCttz(Type *) const override { return !Is64Bit; }
  bool isIntDivCheap(EVT VT, AttributeList) const override {
    return !Is64Bit && VT == MVT::i32;
  }
  bool ShouldShrinkFPConstant(EVT) const override { return false; }
  EVT getSetCCResultType(const DataLayout &DL, LLVMContext &Context,
                         EVT VT) const override;
  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      LLVMContext &Context,
                      const Type *RetTy) const override;

private:
  SDValue LowerCall(CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVAARG(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerCallResult(SDValue Chain, SDValue Glue,
                          CallingConv::ID CallConv, bool IsVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          const SDLoc &DL, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;
  SDValue LowerFormalArguments(
      SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
      const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
      SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;
};

} // namespace llvm

#endif
