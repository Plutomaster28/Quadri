#include "SeaBird.h"
#include "SeaBirdTargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define DEBUG_TYPE "seabird-isel"
#define PASS_NAME "SeaBird DAG-to-DAG Instruction Selection"

namespace {

class SeaBirdDAGToDAGISel final : public SelectionDAGISel {
public:
  explicit SeaBirdDAGToDAGISel(SeaBirdTargetMachine &TM)
      : SelectionDAGISel(TM) {}

private:
  bool selectAddr(SDValue Addr, SDValue &Base, SDValue &Index,
                  SDValue &Scale, SDValue &Disp) {
    SDLoc DL(Addr);
    const MVT PtrVT =
        getTargetLowering()->getPointerTy(CurDAG->getDataLayout());
    if (auto *FI = dyn_cast<FrameIndexSDNode>(Addr)) {
      Base = CurDAG->getTargetFrameIndex(
          FI->getIndex(), PtrVT);
    } else {
      Base = Addr;
    }
    Index = CurDAG->getRegister(SeaBird::R4, PtrVT);
    Scale = CurDAG->getTargetConstant(1, DL, PtrVT);
    Disp = CurDAG->getTargetConstant(0, DL, PtrVT);
    return true;
  }

#include "SeaBirdGenDAGISel.inc"

  void Select(SDNode *Node) override {
    if (Node->isMachineOpcode()) {
      Node->setNodeId(-1);
      return;
    }
    if (Node->getOpcode() == ISD::FrameIndex) {
      auto *FI = cast<FrameIndexSDNode>(Node);
      SDLoc DL(Node);
      const MVT PtrVT =
          getTargetLowering()->getPointerTy(CurDAG->getDataLayout());
      SDValue Ops[] = {
          CurDAG->getTargetFrameIndex(FI->getIndex(), PtrVT),
          CurDAG->getRegister(SeaBird::R4, PtrVT),
          CurDAG->getTargetConstant(1, DL, PtrVT),
          CurDAG->getTargetConstant(0, DL, PtrVT)};
      const unsigned Opcode =
          PtrVT == MVT::i64 ? SeaBird::LEA : SeaBird::LEA32;
      ReplaceNode(Node, CurDAG->getMachineNode(Opcode, DL, PtrVT, Ops));
      return;
    }
    if (Node->getOpcode() == ISD::GlobalAddress) {
      auto *GA = cast<GlobalAddressSDNode>(Node);
      SDLoc DL(Node);
      const MVT PtrVT =
          getTargetLowering()->getPointerTy(CurDAG->getDataLayout());
      SDValue Address = CurDAG->getTargetGlobalAddress(
          GA->getGlobal(), DL, PtrVT, GA->getOffset());
      const unsigned Opcode =
          PtrVT == MVT::i64 ? SeaBird::MOVI64 : SeaBird::MOVI32;
      ReplaceNode(Node,
                  CurDAG->getMachineNode(Opcode, DL, PtrVT, Address));
      return;
    }
    if (Node->getOpcode() == ISD::ExternalSymbol) {
      auto *ES = cast<ExternalSymbolSDNode>(Node);
      SDLoc DL(Node);
      const MVT PtrVT =
          getTargetLowering()->getPointerTy(CurDAG->getDataLayout());
      SDValue Address =
          CurDAG->getTargetExternalSymbol(ES->getSymbol(), PtrVT);
      const unsigned Opcode =
          PtrVT == MVT::i64 ? SeaBird::MOVI64 : SeaBird::MOVI32;
      ReplaceNode(Node,
                  CurDAG->getMachineNode(Opcode, DL, PtrVT, Address));
      return;
    }
    if (Node->getOpcode() == ISD::ConstantPool) {
      auto *CP = cast<ConstantPoolSDNode>(Node);
      SDLoc DL(Node);
      const MVT PtrVT =
          getTargetLowering()->getPointerTy(CurDAG->getDataLayout());
      SDValue Address =
          CP->isMachineConstantPoolEntry()
              ? CurDAG->getTargetConstantPool(
                    CP->getMachineCPVal(), PtrVT, CP->getAlign(),
                    CP->getOffset(), CP->getTargetFlags())
              : CurDAG->getTargetConstantPool(
                    CP->getConstVal(), PtrVT, CP->getAlign(),
                    CP->getOffset(), CP->getTargetFlags());
      const unsigned Opcode =
          PtrVT == MVT::i64 ? SeaBird::MOVI64 : SeaBird::MOVI32;
      ReplaceNode(Node,
                  CurDAG->getMachineNode(Opcode, DL, PtrVT, Address));
      return;
    }
    SelectCode(Node);
  }
};

class SeaBirdDAGToDAGISelLegacy final : public SelectionDAGISelLegacy {
public:
  static char ID;
  explicit SeaBirdDAGToDAGISelLegacy(SeaBirdTargetMachine &TM)
      : SelectionDAGISelLegacy(
            ID, std::make_unique<SeaBirdDAGToDAGISel>(TM)) {}
};

} // namespace

char SeaBirdDAGToDAGISelLegacy::ID = 0;

INITIALIZE_PASS(SeaBirdDAGToDAGISelLegacy, DEBUG_TYPE, PASS_NAME, false, false)

FunctionPass *llvm::createSeaBirdISelDag(SeaBirdTargetMachine &TM) {
  return new SeaBirdDAGToDAGISelLegacy(TM);
}
