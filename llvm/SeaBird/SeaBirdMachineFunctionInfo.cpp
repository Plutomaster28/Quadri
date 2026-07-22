#include "SeaBirdMachineFunctionInfo.h"

using namespace llvm;

MachineFunctionInfo *
SeaBirdMachineFunctionInfo::clone(
    BumpPtrAllocator &Allocator, MachineFunction &DestMF,
    const DenseMap<MachineBasicBlock *, MachineBasicBlock *> &Src2DstMBB)
    const {
  return DestMF.cloneInfo<SeaBirdMachineFunctionInfo>(*this);
}
