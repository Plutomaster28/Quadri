#ifndef LLVM_LIB_TARGET_SEABIRD_SEABIRDMACHINEFUNCTIONINFO_H
#define LLVM_LIB_TARGET_SEABIRD_SEABIRDMACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

class SeaBirdMachineFunctionInfo final : public MachineFunctionInfo {
  int VarArgsFrameIndex = 0;

public:
  explicit SeaBirdMachineFunctionInfo(const Function &F,
                                      const TargetSubtargetInfo *STI) {}

  MachineFunctionInfo *
  clone(BumpPtrAllocator &Allocator, MachineFunction &DestMF,
        const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
      const override;

  void setVarArgsFrameIndex(int FI) { VarArgsFrameIndex = FI; }
  int getVarArgsFrameIndex() const { return VarArgsFrameIndex; }
};

} // namespace llvm

#endif
