// SeaBird LLVM target description. Generated opcode records are sourced from
// spec/seabird-isa.json; ABI and register policy remain hand-reviewed here.

include "llvm/Target/Target.td"

def FeatureTritium : SubtargetFeature<
    "tritium", "HasTritium", "true",
    "Select the Tritium embedded implementation profile">;
def FeatureTetra : SubtargetFeature<
    "tetra", "HasTetra", "true", "Enable 32-bit Tetra execution">;
def FeatureMPU : SubtargetFeature<
    "mpu", "HasMPU", "true", "Enable the region MPU">;
def FeatureAtomics : SubtargetFeature<
    "atomics", "HasAtomics", "true", "Enable Tritium atomic instructions">;
def FeatureFixedMulDiv : SubtargetFeature<
    "fixed-muldiv", "HasFixedMulDiv", "true",
    "Use fixed-latency integer multiply and divide">;

include "SeaBirdRegisterInfo.td"
include "SeaBirdCallingConv.td"
include "SeaBirdInstrInfo.td"

defm : RemapAllTargetPseudoPointerOperands<GPR64>;

def SeaBirdSchedModel : SchedMachineModel {
  let IssueWidth = 1;
  let MicroOpBufferSize = 0;
  let LoadLatency = 3;
  let CompleteModel = 0;
}

def : ProcessorModel<"generic", SeaBirdSchedModel, []>;
def : ProcessorModel<"seabird-gold", SeaBirdSchedModel, []>;
def : ProcessorModel<"seabird-platinum", SeaBirdSchedModel, []>;
def : ProcessorModel<"tritium-v1", SeaBirdSchedModel,
    [FeatureTritium, FeatureTetra, FeatureMPU, FeatureAtomics,
     FeatureFixedMulDiv]>;

def SeaBirdInstrInfo : InstrInfo;

def SeaBirdAsmWriter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

def SeaBird : Target {
  let InstructionSet = SeaBirdInstrInfo;
  let AssemblyWriters = [SeaBirdAsmWriter];
}
