// Variable-length SeaBird encodings are emitted by a custom MCCodeEmitter.
// Opcode and form fields are generated from the normative ISA database.

def NotTritium : Predicate<"!Subtarget->hasTritium()">,
    AssemblerPredicate<(all_of (not FeatureTritium)),
                       "instruction is excluded from Tritium v1">;
def HasRegisterWindows : Predicate<"Subtarget->hasRegisterWindows()">,
    AssemblerPredicate<(all_of FeatureRegisterWindows),
                       "instruction requires the register-window extension">;

class SeaBirdInst<string mnemonic, string operands, bits<8> opcode,
                  string form, bits<5> formCode> : Instruction {
  let Namespace = "SeaBird";
  let AsmString = mnemonic # "\t" # operands;
  let Size = 0;
  bits<8> Opcode = opcode;
  bits<8> ExtensionGroup = 0;
  bits<3> ScalarWidth = 0;
  string EncodingForm = form;
  let TSFlags{7-0} = opcode;
  let TSFlags{12-8} = formCode;
  let TSFlags{20-13} = ExtensionGroup;
  let TSFlags{23-21} = ScalarWidth;
}

def brtarget : Operand<OtherVT>;
def calltarget : Operand<i64>;
def MemoryOperand : AsmOperandClass {
  let Name = "Memory";
  let ParserMethod = "parseMemoryOperand";
}
def memaddr : Operand<i64> {
  let ParserMatchClass = MemoryOperand;
  let PrintMethod = "printMemoryOperand";
  let MIOperandInfo = (ops GPR64:$base, GPR64:$index,
                           i64imm:$scale, i64imm:$disp);
}
def SBAddr : ComplexPattern<i64, 4, "selectAddr", [frameindex], []>;
def memaddr32 : Operand<i32> {
  let ParserMatchClass = MemoryOperand;
  let PrintMethod = "printMemoryOperand";
  let MIOperandInfo = (ops GPR32:$base, GPR32:$index,
                           i32imm:$scale, i32imm:$disp);
}
def SBAddr32 : ComplexPattern<i32, 4, "selectAddr", [frameindex], []>;

def SeaBirdRetGlue : SDNode<"SeaBirdISD::RET_GLUE", SDTNone,
                             [SDNPHasChain, SDNPOptInGlue, SDNPVariadic,
                              SDNPMayLoad, SDNPMayStore]>;
def SDT_SeaBirdCallSeqStart : SDCallSeqStart<[SDTCisVT<0, iPTR>, SDTCisVT<1, iPTR>]>;
def SDT_SeaBirdCallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, iPTR>, SDTCisVT<1, iPTR>]>;
def SeaBirdCallSeqStart : SDNode<"ISD::CALLSEQ_START", SDT_SeaBirdCallSeqStart,
                                 [SDNPHasChain, SDNPOutGlue]>;
def SeaBirdCallSeqEnd : SDNode<"ISD::CALLSEQ_END", SDT_SeaBirdCallSeqEnd,
                               [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def SDT_SeaBirdCall : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SeaBirdCallNode : SDNode<"SeaBirdISD::CALL", SDT_SeaBirdCall,
                             [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue,
                              SDNPVariadic, SDNPMayLoad, SDNPMayStore]>;
def SDT_SeaBirdCmp : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>, SDTCisInt<0>]>;
def SeaBirdCmpNode : SDNode<"SeaBirdISD::CMP", SDT_SeaBirdCmp,
                            [SDNPHasChain, SDNPOutGlue]>;
def SDT_SeaBirdSetLT : SDTypeProfile<1, 2, [
  SDTCisSameAs<0, 1>, SDTCisSameAs<0, 2>, SDTCisInt<0>
]>;
def SeaBirdSetLTNode : SDNode<"SeaBirdISD::SETLT", SDT_SeaBirdSetLT>;
def SeaBirdMulHNode : SDNode<"SeaBirdISD::MULH", SDT_SeaBirdSetLT>;
def SDT_SeaBirdFPSetCC : SDTypeProfile<1, 3, [
  SDTCisVT<0, i64>, SDTCisSameAs<1, 2>, SDTCisFP<1>, SDTCisVT<3, i64>
]>;
def SeaBirdFPSetCCNode : SDNode<"SeaBirdISD::FPSETCC", SDT_SeaBirdFPSetCC>;
def SDT_SeaBirdBranch : SDTypeProfile<0, 1, [SDTCisVT<0, OtherVT>]>;
def SeaBirdBrEqNode : SDNode<"SeaBirdISD::BR_EQ", SDT_SeaBirdBranch,
                             [SDNPHasChain, SDNPInGlue]>;
def SeaBirdBrNeNode : SDNode<"SeaBirdISD::BR_NE", SDT_SeaBirdBranch,
                             [SDNPHasChain, SDNPInGlue]>;
def SeaBirdBrGtNode : SDNode<"SeaBirdISD::BR_GT", SDT_SeaBirdBranch,
                             [SDNPHasChain, SDNPInGlue]>;
def SeaBirdBrGeNode : SDNode<"SeaBirdISD::BR_GE", SDT_SeaBirdBranch,
                             [SDNPHasChain, SDNPInGlue]>;
def SeaBirdBrLtNode : SDNode<"SeaBirdISD::BR_LT", SDT_SeaBirdBranch,
                             [SDNPHasChain, SDNPInGlue]>;
def SeaBirdBrLeNode : SDNode<"SeaBirdISD::BR_LE", SDT_SeaBirdBranch,
                             [SDNPHasChain, SDNPInGlue]>;
def SeaBirdBrCNode : SDNode<"SeaBirdISD::BR_C", SDT_SeaBirdBranch,
                            [SDNPHasChain, SDNPInGlue]>;
def SeaBirdBrNcNode : SDNode<"SeaBirdISD::BR_NC", SDT_SeaBirdBranch,
                             [SDNPHasChain, SDNPInGlue]>;

class SeaBirdPseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  let Namespace = "SeaBird";
  let OutOperandList = outs;
  let InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
  let isPseudo = 1;
  let Size = 0;
}

let Defs = [R7], Uses = [R7], isCodeGenOnly = 1 in {
def ADJCALLSTACKDOWN : SeaBirdPseudo<(outs), (ins i64imm:$amt1, i64imm:$amt2),
    "# ADJCALLSTACKDOWN $amt1, $amt2", [(SeaBirdCallSeqStart timm:$amt1, timm:$amt2)]>;
def ADJCALLSTACKUP : SeaBirdPseudo<(outs), (ins i64imm:$amt1, i64imm:$amt2),
    "# ADJCALLSTACKUP $amt1, $amt2", [(SeaBirdCallSeqEnd timm:$amt1, timm:$amt2)]>;
}

let usesCustomInserter = 1, isCodeGenOnly = 1 in {
def FPSETCC32 : SeaBirdPseudo<
    (outs GPR64:$dst), (ins FPR32:$lhs, FPR32:$rhs, i64imm:$cc),
    "# FPSETCC32 $dst, $lhs, $rhs, $cc",
    [(set GPR64:$dst,
      (SeaBirdFPSetCCNode FPR32:$lhs, FPR32:$rhs, (i64 imm:$cc)))]>;
def FPSETCC64 : SeaBirdPseudo<
    (outs GPR64:$dst), (ins FPR64:$lhs, FPR64:$rhs, i64imm:$cc),
    "# FPSETCC64 $dst, $lhs, $rhs, $cc",
    [(set GPR64:$dst,
      (SeaBirdFPSetCCNode FPR64:$lhs, FPR64:$rhs, (i64 imm:$cc)))]>;
def FPSETCC128 : SeaBirdPseudo<
    (outs GPR64:$dst), (ins FPR128:$lhs, FPR128:$rhs, i64imm:$cc),
    "# FPSETCC128 $dst, $lhs, $rhs, $cc",
    [(set GPR64:$dst,
      (SeaBirdFPSetCCNode FPR128:$lhs, FPR128:$rhs, (i64 imm:$cc)))]>;
def FPSELECT32 : SeaBirdPseudo<
    (outs FPR32:$dst),
    (ins GPR64:$cond, FPR32:$true_value, FPR32:$false_value),
    "# FPSELECT32 $dst, $cond, $true_value, $false_value",
    [(set FPR32:$dst,
      (select GPR64:$cond, FPR32:$true_value, FPR32:$false_value))]>;
def FPSELECT64 : SeaBirdPseudo<
    (outs FPR64:$dst),
    (ins GPR64:$cond, FPR64:$true_value, FPR64:$false_value),
    "# FPSELECT64 $dst, $cond, $true_value, $false_value",
    [(set FPR64:$dst,
      (select GPR64:$cond, FPR64:$true_value, FPR64:$false_value))]>;
def FPSELECT128 : SeaBirdPseudo<
    (outs FPR128:$dst),
    (ins GPR64:$cond, FPR128:$true_value, FPR128:$false_value),
    "# FPSELECT128 $dst, $cond, $true_value, $false_value",
    [(set FPR128:$dst,
      (select GPR64:$cond, FPR128:$true_value, FPR128:$false_value))]>;
}


class CopyRR<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type", 1> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$src);
  let isMoveReg = 1;
}

class CopyVV<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "V-type copy (128)", 12> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$src);
  let isMoveReg = 1;
}

class ALURR<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type", 2> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$old, GPR64:$src);
  let Constraints = "$dst = $old";
}

class CarryALURR<string mnemonic, bits<8> opcode>
    : ALURR<mnemonic, opcode> {
  let Uses = [FLAGS];
  let Defs = [FLAGS];
}

class UnaryInPlace<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst", opcode, "R-type (unary)", 20> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$old);
  let Constraints = "$dst = $old";
}

class UnaryRR<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type", 21> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$src);
}

class CompareRR<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$lhs, $rhs", opcode, "R-type", 3> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$lhs, GPR64:$rhs);
  let Defs = [FLAGS];
}

class RI64<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $imm", opcode, "I-type (8B)", 4> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins i64imm:$imm);
}

class ALURI<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $imm", opcode, "I-type (8B)", 22> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$old, i64imm:$imm);
  let Constraints = "$dst = $old";
}

class CompareRI<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$lhs, $imm", opcode, "I-type (8B)", 23> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$lhs, i64imm:$imm);
  let Defs = [FLAGS];
}

class TernaryRI<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src, $imm", opcode,
                  "R-type + immediate", 28> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$src, i64imm:$imm);
}

class TernaryByte<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src, $imm", opcode,
                  "R-type + XOP immediate", 29> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$src, i8imm:$imm);
}

class TernaryRR<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src, $extra", opcode,
                  "R-type + XOP register", 10> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$src, GPR64:$extra);
}

class StringCopy<string mnemonic, bits<8> opcode>
    : TernaryRR<mnemonic, opcode> {
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class StringFill<string mnemonic, bits<8> opcode>
    : TernaryRR<mnemonic, opcode> {
  let mayStore = 1;
  let hasSideEffects = 1;
}

class PairLoad<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst1, $dst2, $address", opcode,
                  "M-type + pair", 10> {
  dag OutOperandList = (outs GPR64:$dst1, GPR64:$dst2);
  dag InOperandList = (ins memaddr:$address);
  let mayLoad = 1;
}

class PairStore<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$address, $src1, $src2", opcode,
                  "M-type + pair", 8> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$address, GPR64:$src1, GPR64:$src2);
  let mayStore = 1;
}

class LoadQ<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $base", opcode, "M-type (8B)", 7> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins memaddr:$base);
  let mayLoad = 1;
}

class StoreQ<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$base, $src", opcode, "M-type (8B)", 8> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$base, GPR64:$src);
  let mayStore = 1;
}

class AddressCalc<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $base", opcode, "M-type address", 15> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins memaddr:$base);
}

class FPBinary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode, "R-type (FP64)", 10> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList = (ins FPR64:$lhs, FPR64:$rhs);
}

// Unary FP and FP compare encodings use the same ModR/M + VectorCtl byte
// layout as a two-register vector copy. Keeping that shared MC form avoids
// consuming another TSFlags form value while preserving their distinct
// register-def and FLAGS semantics in TableGen.
class FPUnary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type (FP64)", 12> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList = (ins FPR64:$src);
}

class FPCompare<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$lhs, $rhs", opcode, "R-type (FP64)", 12> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins FPR64:$lhs, FPR64:$rhs);
  let Defs = [FLAGS];
  let hasSideEffects = 1;
}

class FPXFused<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $mul_lhs, $mul_rhs, $addend", opcode,
                  "FPX fused scalar", 10> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList =
      (ins FPR64:$mul_lhs, FPR64:$mul_rhs, FPR64:$addend);
  let ExtensionGroup = 5;
}

class FPXBinary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode,
                  "FPX scalar binary", 10> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList = (ins FPR64:$lhs, FPR64:$rhs);
  let ExtensionGroup = 5;
}

class FPXUnary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "FPX scalar unary", 12> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList = (ins FPR64:$src);
  let ExtensionGroup = 5;
}

class FPXLoad<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $base", opcode, "FPX scalar load (64)", 13> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList = (ins memaddr:$base);
  let ExtensionGroup = 5;
  let mayLoad = 1;
}

class FPXStore<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$base, $src", opcode,
                  "FPX scalar store (64)", 14> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$base, FPR64:$src);
  let ExtensionGroup = 5;
  let mayStore = 1;
}

class CryptoBinary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode,
                  "CRYPTO vector binary", 11> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$lhs, VR128:$rhs);
  let ExtensionGroup = 2;
}

class CryptoUnary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode,
                  "CRYPTO vector unary", 12> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$src);
  let ExtensionGroup = 2;
}

class DSPTernary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode,
                  "DSP GPR ternary", 10> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$lhs, GPR64:$rhs);
  let ExtensionGroup = 3;
}

class DSPQuaternary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $first, $second, $third", opcode,
                  "DSP GPR quaternary", 10> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList =
      (ins GPR64:$first, GPR64:$second, GPR64:$third);
  let ExtensionGroup = 3;
}

class DSPPairOut<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst_hi, $dst_lo, $lhs, $rhs", opcode,
                  "DSP GPR pair output", 10> {
  dag OutOperandList = (outs GPR64:$dst_hi, GPR64:$dst_lo);
  dag InOperandList = (ins GPR64:$lhs, GPR64:$rhs);
  let ExtensionGroup = 3;
}

class DSPUnary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "DSP GPR unary", 12> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$src);
  let ExtensionGroup = 3;
}

class DSPRegImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src, $imm", opcode,
                  "DSP GPR + immediate", 29> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$src, i8imm:$imm);
  let ExtensionGroup = 3;
}

class DSPDot<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode,
                  "DSP vector dot product", 11> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins VR128:$lhs, VR128:$rhs);
  let ExtensionGroup = 3;
}

class DSPSumDot<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode,
                  "DSP vector horizontal sum", 12> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins VR128:$src);
  let ExtensionGroup = 3;
}

class AVXBinary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode,
                  "AVX vector binary", 11> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$lhs, VR128:$rhs);
  let ExtensionGroup = 1;
}

class AVXUnary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode,
                  "AVX vector unary", 12> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$src);
  let ExtensionGroup = 1;
}

class AVXRegImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src, $imm", opcode,
                  "AVX vector immediate", 29> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$src, i8imm:$imm);
  let ExtensionGroup = 1;
}

class AVXBinaryImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs, $imm", opcode,
                  "AVX vector binary + immediate", 10> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$lhs, VR128:$rhs, i8imm:$imm);
  let ExtensionGroup = 1;
}

class AVXGather<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $base, $mask", opcode,
                  "AVX gather memory", 16> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins memaddr:$base, i8imm:$mask);
  let ExtensionGroup = 1;
  let mayLoad = 1;
}

class AVXScatter<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$base, $src, $mask", opcode,
                  "AVX scatter memory", 17> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$base, VR128:$src, i8imm:$mask);
  let ExtensionGroup = 1;
  let mayStore = 1;
}

class AVXNoOperand<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "", opcode, "AVX no operand", 6> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins);
  let ExtensionGroup = 1;
  let hasSideEffects = 1;
}

class VBinary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode, "V-type (128x64)", 11> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$lhs, VR128:$rhs);
}

class VUnary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "V-type (128x64)", 12> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$src);
}

// The encoder specializes the shared ternary-byte MC form for vector
// ModR/M + VectorCtl + immediate layout.
class VShiftImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src, $imm", opcode,
                  "V-type + immediate (128x64)", 29> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins VR128:$src, i8imm:$imm);
}

class ScalarFPLoad<string mnemonic, bits<8> subopcode>
    : SeaBirdInst<mnemonic, "$dst, $base", subopcode, "FPX scalar load (64)", 13> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList = (ins memaddr:$base);
  let mayLoad = 1;
}

class ScalarFPStore<string mnemonic, bits<8> subopcode>
    : SeaBirdInst<mnemonic, "$base, $src", subopcode, "FPX scalar store (64)", 14> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$base, FPR64:$src);
  let mayStore = 1;
}

class VectorLoad<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $base", opcode, "V-type memory (128)", 16> {
  dag OutOperandList = (outs VR128:$dst);
  dag InOperandList = (ins memaddr:$base);
  let mayLoad = 1;
}

class VectorStore<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$base, $src", opcode, "V-type memory (128)", 17> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$base, VR128:$src);
  let mayStore = 1;
}

class IntToFP<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type i64-to-f64", 18> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList = (ins GPR64:$src);
}

class FPToInt<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type f64-to-i64", 19> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins FPR64:$src);
}

class UncondBranch<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$target", opcode, "B-type (rel32)", 5> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins brtarget:$target);
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

class CondBranch<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$target", opcode, "B-type (rel32)", 5> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins brtarget:$target);
  let isBranch = 1;
  let isTerminator = 1;
  let Uses = [FLAGS];
}

class RegCondBranch<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$reg, $target", opcode, "B-type (reg)", 25> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$reg, brtarget:$target);
  let isBranch = 1;
  let isTerminator = 1;
}

class Call<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$target", opcode, "B-type (rel32)", 5> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins calltarget:$target);
  let isCall = 1;
  let Defs = [R7];
  let Uses = [R7];
  // CALL always pushes a return address. A window transition may also spill
  // or restore transparent backing state, so model both directions.
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class IndirectCall<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$target", opcode, "B-type (abs)", 9> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$target);
  let isCall = 1;
  let Defs = [R7];
  let Uses = [R7];
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class IndirectBranch<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$target", opcode, "B-type (abs)", 9> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$target);
  let isBranch = 1;
  let isTerminator = 1;
  let isBarrier = 1;
}

class TrapImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$imm", opcode, "S-type", 24> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins i8imm:$imm);
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class PushReg<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$src", opcode, "S-type", 26> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$src);
  let Defs = [R7];
  let Uses = [R7];
  let mayStore = 1;
  let hasSideEffects = 1;
}

class PopReg<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst", opcode, "S-type", 26> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins);
  let Defs = [R7];
  let Uses = [R7];
  let mayLoad = 1;
  let hasSideEffects = 1;
}

class FrameImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$size", opcode, "X-type", 27> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins i64imm:$size);
  let Defs = [R6, R7];
  let Uses = [R6, R7];
  let mayStore = 1;
  let hasSideEffects = 1;
}

class SleepImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$duration", opcode, "S-type + immediate", 27> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins i64imm:$duration);
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class ReadReg<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst", opcode, "S-type", 30> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins);
  let hasSideEffects = 1;
}

class ReadControl<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $control", opcode, "S-type", 31> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins i16imm:$control);
  let hasSideEffects = 1;
}

class WriteControl<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$control, $src", opcode, "S-type", 31> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins i16imm:$control, GPR64:$src);
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class SystemNoOperand<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "", opcode, "S-type", 6> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins);
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class SysXQuery<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$leaf, $subleaf", opcode, "SYSX BASE", 3> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$leaf, GPR64:$subleaf);
  let ExtensionGroup = 4;
  let Defs = [R0, R1, R2, R3];
  let hasSideEffects = 1;
}

class SysXReadImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $selector", opcode,
                  "SYSX register + immediate", 31> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins i16imm:$selector);
  let ExtensionGroup = 4;
  let hasSideEffects = 1;
}

class SysXWriteImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$selector, $src", opcode,
                  "SYSX immediate + register", 31> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins i16imm:$selector, GPR64:$src);
  let ExtensionGroup = 4;
  let hasSideEffects = 1;
}

class SysXMaskedMemory<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$address, $mask", opcode,
                  "SYSX memory + register mask", 8> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$address, GPR64:$mask);
  let ExtensionGroup = 4;
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class SysXNoOperand<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "", opcode, "SYSX no operand", 6> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins);
  let ExtensionGroup = 4;
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class WindowTransition<string mnemonic, bits<8> opcode>
    : SysXNoOperand<mnemonic, opcode> {
  let mayLoad = 1;
  let mayStore = 1;
}

class SysXPair<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$lhs, $rhs", opcode, "SYSX register pair", 3> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$lhs, GPR64:$rhs);
  let ExtensionGroup = 4;
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class SysXTernary<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$first, $second, $third", opcode,
                  "SYSX register triple", 10> {
  dag OutOperandList = (outs);
  dag InOperandList =
      (ins GPR64:$first, GPR64:$second, GPR64:$third);
  let ExtensionGroup = 4;
  let hasSideEffects = 1;
}

class SysXReg<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$reg", opcode, "SYSX register", 26> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$reg);
  let ExtensionGroup = 4;
  let hasSideEffects = 1;
}

class SysXReadReg<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst", opcode, "SYSX register", 30> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins);
  let ExtensionGroup = 4;
  let hasSideEffects = 1;
}

class SysXMemory<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$address", opcode, "SYSX memory", 7> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$address);
  let ExtensionGroup = 4;
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class SysXStoreMemory<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$address, $src", opcode, "SYSX memory store", 8> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$address, GPR64:$src);
  let ExtensionGroup = 4;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class SysXImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$value", opcode, "SYSX immediate", 24> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins i8imm:$value);
  let ExtensionGroup = 4;
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class AtomicMem<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $address", opcode, "M-type atomic", 7> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins GPR64:$old, memaddr:$address);
  let Constraints = "$dst = $old";
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class AtomicCAS<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$expected, $desired, $address", opcode,
                  "M-type atomic + XOP", 7> {
  dag OutOperandList = (outs GPR64:$expected);
  dag InOperandList =
      (ins GPR64:$old_expected, GPR64:$desired, memaddr:$address);
  let Constraints = "$expected = $old_expected";
  let Defs = [FLAGS];
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class AtomicExchange<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$value, $address", opcode,
                  "M-type atomic exchange", 7> {
  dag OutOperandList = (outs GPR64:$value);
  dag InOperandList = (ins GPR64:$old_value, memaddr:$address);
  let Constraints = "$value = $old_value";
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class AtomicExchange128<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$value, $address", opcode,
                  "M-type atomic vector exchange", 16> {
  dag OutOperandList = (outs VR128:$value);
  dag InOperandList = (ins VR128:$old_value, memaddr:$address);
  let Constraints = "$value = $old_value";
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

class AtomicLoad<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $address", opcode, "M-type atomic", 7> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins memaddr:$address);
  let mayLoad = 1;
  let hasSideEffects = 1;
}

class MemoryOnly<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$address", opcode, "M-type", 7> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$address);
  let mayLoad = 1;
  let hasSideEffects = 1;
}

class NoOperand<string mnemonic, bits<8> opcode, string form>
    : SeaBirdInst<mnemonic, "", opcode, form, 6> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins);
}

class TxnBeginRel<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$target", opcode, "TXN rel32", 5> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins brtarget:$target);
  let hasSideEffects = 1;
}

class TxnBeginAbs<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$target", opcode, "TXN absolute register", 9> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$target);
  let hasSideEffects = 1;
}

class TxnNoOperand<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "", opcode, "TXN no operand", 6> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins);
  let hasSideEffects = 1;
}

class TxnAbortImm<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$code", opcode, "TXN abort immediate", 24> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins i8imm:$code);
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class TxnAbortReg<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$code", opcode, "TXN abort register", 26> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR64:$code);
  let hasSideEffects = 1;
  let isBarrier = 1;
}

class TxnReadReg<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst", opcode, "TXN status register", 30> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins);
  let hasSideEffects = 1;
}


class Return<string mnemonic, bits<8> opcode>
    : NoOperand<mnemonic, opcode, "B-type"> {
  let isReturn = 1;
  let isTerminator = 1;
  let isBarrier = 1;
  let Defs = [R7];
  let Uses = [R7];
  let mayLoad = 1;
  // Window restoration may write a transparent spill record.
  let mayStore = 1;
  let Pattern = [(SeaBirdRetGlue)];
}

include "SeaBirdGenOpcodes.td"

// Binary32 uses the same architectural opcodes and vector registers as
// binary64, with the scalar format selected by the instruction prefix. These
// records use an explicit `.s` spelling so the textual assembler can select
// binary32 while the unsuffixed normative spelling continues to mean binary64.
let Predicates = [NotTritium] in {
class FPBinary32CG<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode, "R-type (FP32)", 10> {
  dag OutOperandList = (outs FPR32:$dst);
  dag InOperandList = (ins FPR32:$lhs, FPR32:$rhs);
  let ScalarWidth = 3;
}
class FPUnary32CG<string mnemonic, bits<8> opcode, bits<8> group = 0>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type (FP32)", 12> {
  dag OutOperandList = (outs FPR32:$dst);
  dag InOperandList = (ins FPR32:$src);
  let ScalarWidth = 3;
  let ExtensionGroup = group;
}
class FPCompare32CG<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$lhs, $rhs", opcode, "R-type (FP32)", 12> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins FPR32:$lhs, FPR32:$rhs);
  let ScalarWidth = 3;
  let Defs = [FLAGS];
  let hasSideEffects = 1;
}
class FPXFused32CG<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $mul_lhs, $mul_rhs, $addend", opcode,
                  "FPX fused scalar (FP32)", 10> {
  dag OutOperandList = (outs FPR32:$dst);
  dag InOperandList =
      (ins FPR32:$mul_lhs, FPR32:$mul_rhs, FPR32:$addend);
  let ScalarWidth = 3;
  let ExtensionGroup = 5;
}
class FPXBinary32CG<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode,
                  "FPX scalar binary (FP32)", 10> {
  dag OutOperandList = (outs FPR32:$dst);
  dag InOperandList = (ins FPR32:$lhs, FPR32:$rhs);
  let ScalarWidth = 3;
  let ExtensionGroup = 5;
}
class ScalarFPLoad32CG
    : SeaBirdInst<"fld.s", "$dst, $base", 0x10, "FPX scalar load (32)", 13> {
  dag OutOperandList = (outs FPR32:$dst);
  dag InOperandList = (ins memaddr:$base);
  let ScalarWidth = 3;
  let ExtensionGroup = 5;
  let mayLoad = 1;
}
class ScalarFPStore32CG
    : SeaBirdInst<"fst.s", "$base, $src", 0x11, "FPX scalar store (32)", 14> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$base, FPR32:$src);
  let ScalarWidth = 3;
  let ExtensionGroup = 5;
  let mayStore = 1;
}
class IntToFP32CG
    : SeaBirdInst<"fcvti.s", "$dst, $src", 0xAC, "R-type i64-to-f32", 18> {
  dag OutOperandList = (outs FPR32:$dst);
  dag InOperandList = (ins GPR64:$src);
  let ScalarWidth = 3;
}
class FP32ToIntCG
    : SeaBirdInst<"fcvts.s", "$dst, $src", 0xAD, "R-type f32-to-i64", 19> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins FPR32:$src);
  let ScalarWidth = 3;
}
class UIntToFP32CG
    : SeaBirdInst<"fcvtu.s", "$dst, $src", 0xDB, "R-type u64-to-f32", 18> {
  dag OutOperandList = (outs FPR32:$dst);
  dag InOperandList = (ins GPR64:$src);
  let ScalarWidth = 3;
}
class FP32ToUIntCG
    : SeaBirdInst<"fcvtus.s", "$dst, $src", 0xDC, "R-type f32-to-u64", 19> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins FPR32:$src);
  let ScalarWidth = 3;
}
class FPExt32To64CG
    : SeaBirdInst<"fcvt.s2d", "$dst, $src", 0x0A, "FPX f32-to-f64", 12> {
  dag OutOperandList = (outs FPR64:$dst);
  dag InOperandList = (ins FPR32:$src);
  let ExtensionGroup = 5;
  let isCodeGenOnly = 1;
}
class FPRound64To32CG
    : SeaBirdInst<"fcvt.d2s", "$dst, $src", 0x0B, "FPX f64-to-f32", 12> {
  dag OutOperandList = (outs FPR32:$dst);
  dag InOperandList = (ins FPR64:$src);
  let ExtensionGroup = 5;
  let isCodeGenOnly = 1;
}

def FADD32CG : FPBinary32CG<"fadd.s", 0xA6>;
def FSUB32CG : FPBinary32CG<"fsub.s", 0xA7>;
def FMUL32CG : FPBinary32CG<"fmul.s", 0xA8>;
def FDIV32CG : FPBinary32CG<"fdiv.s", 0xA9>;
def FNEG32CG : FPUnary32CG<"fneg.s", 0xAE>;
def FABS32CG : FPUnary32CG<"fabs.s", 0xAF>;
def FSQRT32CG : FPUnary32CG<"fsqrt.s", 0xAA>;
def FCMP32CG : FPCompare32CG<"fcmp.s", 0xAB>;
def FMADD32CG : FPXFused32CG<"fmadd.s", 0x00>;
def FMSUB32CG : FPXFused32CG<"fmsub.s", 0x01>;
def FMIN32CG : FPXBinary32CG<"fmin.s", 0x04>;
def FMAX32CG : FPXBinary32CG<"fmax.s", 0x05>;
def FLD32CG : ScalarFPLoad32CG;
def FST32CG : ScalarFPStore32CG;
def FCVTI32CG : IntToFP32CG;
def FCVTS32CG : FP32ToIntCG;
def FCVTU32CG : UIntToFP32CG;
def FCVTUS32CG : FP32ToUIntCG;
def FCVT_S2D32CG : FPExt32To64CG;
def FCVT_D2S32CG : FPRound64To32CG;
}

// Binary128 is encoded by the same scalar FP instructions with a 16-byte
// lane/operand-width prefix. `.q` keeps the assembly spelling distinct from
// the normative unsuffixed binary64 form.
let Predicates = [NotTritium] in {
class FPBinary128CG<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode, "R-type (FP128)", 10> {
  dag OutOperandList = (outs FPR128:$dst);
  dag InOperandList = (ins FPR128:$lhs, FPR128:$rhs);
  let ScalarWidth = 5;
}
class FPUnary128CG<string mnemonic, bits<8> opcode, bits<8> group = 0>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type (FP128)", 12> {
  dag OutOperandList = (outs FPR128:$dst);
  dag InOperandList = (ins FPR128:$src);
  let ScalarWidth = 5;
  let ExtensionGroup = group;
}
class FPCompare128CG<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$lhs, $rhs", opcode, "R-type (FP128)", 12> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins FPR128:$lhs, FPR128:$rhs);
  let ScalarWidth = 5;
  let Defs = [FLAGS];
  let hasSideEffects = 1;
}
class FPXFused128CG<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $mul_lhs, $mul_rhs, $addend", opcode,
                  "FPX fused scalar (FP128)", 10> {
  dag OutOperandList = (outs FPR128:$dst);
  dag InOperandList =
      (ins FPR128:$mul_lhs, FPR128:$mul_rhs, FPR128:$addend);
  let ScalarWidth = 5;
  let ExtensionGroup = 5;
}
class FPXBinary128CG<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $lhs, $rhs", opcode,
                  "FPX scalar binary (FP128)", 10> {
  dag OutOperandList = (outs FPR128:$dst);
  dag InOperandList = (ins FPR128:$lhs, FPR128:$rhs);
  let ScalarWidth = 5;
  let ExtensionGroup = 5;
}
class ScalarFPLoad128CG
    : SeaBirdInst<"fld.q", "$dst, $base", 0x10, "FPX scalar load (128)", 13> {
  dag OutOperandList = (outs FPR128:$dst);
  dag InOperandList = (ins memaddr:$base);
  let ScalarWidth = 5;
  let ExtensionGroup = 5;
  let mayLoad = 1;
}
class ScalarFPStore128CG
    : SeaBirdInst<"fst.q", "$base, $src", 0x11, "FPX scalar store (128)", 14> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr:$base, FPR128:$src);
  let ScalarWidth = 5;
  let ExtensionGroup = 5;
  let mayStore = 1;
}
class IntToFP128CG
    : SeaBirdInst<"fcvti.q", "$dst, $src", 0xAC, "R-type i64-to-f128", 18> {
  dag OutOperandList = (outs FPR128:$dst);
  dag InOperandList = (ins GPR64:$src);
  let ScalarWidth = 5;
}
class FP128ToIntCG
    : SeaBirdInst<"fcvts.q", "$dst, $src", 0xAD, "R-type f128-to-i64", 19> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins FPR128:$src);
  let ScalarWidth = 5;
}
class UIntToFP128CG
    : SeaBirdInst<"fcvtu.q", "$dst, $src", 0xDB, "R-type u64-to-f128", 18> {
  dag OutOperandList = (outs FPR128:$dst);
  dag InOperandList = (ins GPR64:$src);
  let ScalarWidth = 5;
}
class FP128ToUIntCG
    : SeaBirdInst<"fcvtus.q", "$dst, $src", 0xDC, "R-type f128-to-u64", 19> {
  dag OutOperandList = (outs GPR64:$dst);
  dag InOperandList = (ins FPR128:$src);
  let ScalarWidth = 5;
}

def FADD128CG : FPBinary128CG<"fadd.q", 0xA6>;
def FSUB128CG : FPBinary128CG<"fsub.q", 0xA7>;
def FMUL128CG : FPBinary128CG<"fmul.q", 0xA8>;
def FDIV128CG : FPBinary128CG<"fdiv.q", 0xA9>;
def FNEG128CG : FPUnary128CG<"fneg.q", 0xAE>;
def FABS128CG : FPUnary128CG<"fabs.q", 0xAF>;
def FSQRT128CG : FPUnary128CG<"fsqrt.q", 0xAA>;
def FCMP128CG : FPCompare128CG<"fcmp.q", 0xAB>;
def FMADD128CG : FPXFused128CG<"fmadd.q", 0x00>;
def FMSUB128CG : FPXFused128CG<"fmsub.q", 0x01>;
def FMIN128CG : FPXBinary128CG<"fmin.q", 0x04>;
def FMAX128CG : FPXBinary128CG<"fmax.q", 0x05>;
def FLD128CG : ScalarFPLoad128CG;
def FST128CG : ScalarFPStore128CG;
def FCVTI128CG : IntToFP128CG;
def FCVTS128CG : FP128ToIntCG;
def FCVTU128CG : UIntToFP128CG;
def FCVTUS128CG : FP128ToUIntCG;
}

// Tetra code-generation records share the architectural opcodes and textual
// syntax with the mode-independent MC records above. They are codegen-only so
// the assembler has one unambiguous mnemonic/register matcher.
let isCodeGenOnly = 1 in {
class CopyRR32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type", 1> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins GPR32:$src);
  let isMoveReg = 1;
}
class ALURR32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type", 2> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins GPR32:$old, GPR32:$src);
  let Constraints = "$dst = $old";
}
class UnaryInPlace32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst", opcode, "R-type (unary)", 20> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins GPR32:$old);
  let Constraints = "$dst = $old";
}
class UnaryRR32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src", opcode, "R-type", 21> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins GPR32:$src);
}
class CompareRR32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$lhs, $rhs", opcode, "R-type", 3> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR32:$lhs, GPR32:$rhs);
  let Defs = [FLAGS];
}
class RI32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $imm", opcode, "I-type (4B)", 4> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins i32imm:$imm);
}
class ALURI32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $imm", opcode, "I-type (4B)", 22> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins GPR32:$old, i32imm:$imm);
  let Constraints = "$dst = $old";
}
class CompareRI32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$lhs, $imm", opcode, "I-type (4B)", 23> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR32:$lhs, i32imm:$imm);
  let Defs = [FLAGS];
}
class TernaryRI32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src, $imm", opcode,
                  "R-type + immediate", 28> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins GPR32:$src, i32imm:$imm);
}
class TernaryByte32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $src, $imm", opcode,
                  "R-type + XOP immediate", 29> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins GPR32:$src, i8imm:$imm);
}
class Load32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $base", opcode, "M-type", 7> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins memaddr32:$base);
  let mayLoad = 1;
}
class Store32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$base, $src", opcode, "M-type", 8> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins memaddr32:$base, GPR32:$src);
  let mayStore = 1;
}
class AddressCalc32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $base", opcode, "M-type address", 15> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins memaddr32:$base);
}
class AtomicMem32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$dst, $address", opcode, "M-type atomic", 7> {
  dag OutOperandList = (outs GPR32:$dst);
  dag InOperandList = (ins GPR32:$old, memaddr32:$address);
  let Constraints = "$dst = $old";
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}
class AtomicCAS32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$expected, $desired, $address", opcode,
                  "M-type atomic + XOP", 7> {
  dag OutOperandList = (outs GPR32:$expected);
  dag InOperandList =
      (ins GPR32:$old_expected, GPR32:$desired, memaddr32:$address);
  let Constraints = "$expected = $old_expected";
  let Defs = [FLAGS];
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}
class IndirectCall32<string mnemonic, bits<8> opcode>
    : SeaBirdInst<mnemonic, "$target", opcode, "B-type (abs)", 9> {
  dag OutOperandList = (outs);
  dag InOperandList = (ins GPR32:$target);
  let isCall = 1;
  let Defs = [R7];
  let Uses = [R7];
  let mayLoad = 1;
  let mayStore = 1;
  let hasSideEffects = 1;
}

def MOVrr32 : CopyRR32<"mov", MOVrr.Opcode>;
def MOVI32 : RI32<"movi", MOVI64.Opcode>;
def ADDrr32 : ALURR32<"add", ADDrr.Opcode>;
def ADDI32 : ALURI32<"addi", ADDI.Opcode>;
def SUBrr32 : ALURR32<"sub", SUBrr.Opcode>;
def SUBI32 : ALURI32<"subi", SUBI.Opcode>;
def MULrr32 : ALURR32<"mul", MULrr.Opcode>;
def MULI32 : ALURI32<"muli", MULI.Opcode>;
def DIVrr32 : ALURR32<"div", DIVrr.Opcode>;
def DIVI32 : ALURI32<"divi", DIVI.Opcode>;
def MODrr32 : ALURR32<"mod", MODrr.Opcode>;
def MODI32 : ALURI32<"modi", MODI.Opcode>;
def NEG32 : UnaryInPlace32<"neg", NEG.Opcode>;
def INC32 : UnaryInPlace32<"inc", INC.Opcode>;
def DEC32 : UnaryInPlace32<"dec", DEC.Opcode>;
def MULHrr32 : ALURR32<"mulh", MULHrr.Opcode>;
def UMULrr32 : ALURR32<"umul", UMULrr.Opcode>;
def UDIVrr32 : ALURR32<"udiv", UDIVrr.Opcode>;
def ADDSrr32 : ALURR32<"adds", ADDSrr.Opcode>;
def ADDUrr32 : ALURR32<"addu", ADDUrr.Opcode>;
def SUBSrr32 : ALURR32<"subs", SUBSrr.Opcode>;
def SUBUrr32 : ALURR32<"subu", SUBUrr.Opcode>;
def ABSrr32 : UnaryRR32<"abs", ABSrr.Opcode>;
def CLZrr32 : UnaryRR32<"clz", CLZrr.Opcode>;
def CTZrr32 : UnaryRR32<"ctz", CTZrr.Opcode>;
def POPCrr32 : UnaryRR32<"popc", POPCrr.Opcode>;
def ANDrr32 : ALURR32<"and", ANDrr.Opcode>;
def ORrr32 : ALURR32<"or", ORrr.Opcode>;
def XORrr32 : ALURR32<"xor", XORrr.Opcode>;
def NOT32 : UnaryInPlace32<"not", NOT.Opcode>;
def NANDrr32 : ALURR32<"nand", NANDrr.Opcode>;
def NORrr32 : ALURR32<"nor", NORrr.Opcode>;
def XNORrr32 : ALURR32<"xnor", XNORrr.Opcode>;
def SHLrr32 : ALURR32<"shl", SHLrr.Opcode>;
def SHRrr32 : ALURR32<"shr", SHRrr.Opcode>;
def SARrr32 : ALURR32<"sar", SARrr.Opcode>;
def ROLrr32 : ALURR32<"rol", ROLrr.Opcode>;
def RORrr32 : ALURR32<"ror", RORrr.Opcode>;
def BSETrr32 : ALURR32<"bset", BSETrr.Opcode>;
def BCLRrr32 : ALURR32<"bclr", BCLRrr.Opcode>;
def BTOGrr32 : ALURR32<"btog", BTOGrr.Opcode>;
def BTSTrr32 : CompareRR32<"btst", BTSTrr.Opcode>;
def MASKri32 : TernaryRI32<"mask", MASKri.Opcode>;
def EXTri32 : TernaryByte32<"ext", EXTri.Opcode>;
def CMPrr32 : CompareRR32<"cmp", CMPrr.Opcode>;
def CMPI32 : CompareRI32<"cmpi", CMPI.Opcode>;
def CMPSrr32 : CompareRR32<"cmps", CMPSrr.Opcode>;
def CMPUrr32 : CompareRR32<"cmpu", CMPUrr.Opcode>;
def TSTrr32 : CompareRR32<"tst", TSTrr.Opcode>;
def MAXrr32 : ALURR32<"max", MAXrr.Opcode>;
def MINrr32 : ALURR32<"min", MINrr.Opcode>;
def SLTrr32 : ALURR32<"slt", SLTrr.Opcode>;
def SGTrr32 : ALURR32<"sgt", SGTrr.Opcode>;
def TSTI32 : CompareRI32<"tsti", TSTI.Opcode>;
def LDBrr32 : Load32<"ldb", LDBrr.Opcode>;
def LDHrr32 : Load32<"ldh", LDHrr.Opcode>;
def LDWrr32 : Load32<"ldw", LDWrr.Opcode>;
def STBrr32 : Store32<"stb", STBrr.Opcode>;
def STHrr32 : Store32<"sth", STHrr.Opcode>;
def STWrr32 : Store32<"stw", STWrr.Opcode>;
def LEA32 : AddressCalc32<"lea", LEA.Opcode>;
def CALLA32 : IndirectCall32<"calla", CALLA.Opcode>;
def CMPXCHG32 : AtomicCAS32<"cmpxchg", CMPXCHG.Opcode>;
def ATADD32 : AtomicMem32<"atadd", ATADD.Opcode>;
def ATSUB32 : AtomicMem32<"atsub", ATSUB.Opcode>;
def ATAND32 : AtomicMem32<"atand", ATAND.Opcode>;
def ATOR32 : AtomicMem32<"ator", ATOR.Opcode>;
def ATXOR32 : AtomicMem32<"atxor", ATXOR.Opcode>;
}

def MOVV128 : CopyVV<"mov", 0x00>;
def : Pat<(add GPR64:$lhs, GPR64:$rhs),
          (ADDrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(sub GPR64:$lhs, GPR64:$rhs),
          (SUBrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(and GPR64:$lhs, GPR64:$rhs),
          (ANDrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(or GPR64:$lhs, GPR64:$rhs),
          (ORrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(xor GPR64:$lhs, GPR64:$rhs),
          (XORrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(shl GPR64:$lhs, GPR64:$rhs),
          (SHLrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(srl GPR64:$lhs, GPR64:$rhs),
          (SHRrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(sra GPR64:$lhs, GPR64:$rhs),
          (SARrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(mul GPR64:$lhs, GPR64:$rhs),
          (MULrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(mulhu GPR64:$lhs, GPR64:$rhs),
          (UMULHrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(sdiv GPR64:$lhs, GPR64:$rhs),
          (DIVrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(udiv GPR64:$lhs, GPR64:$rhs),
          (UDIVrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(srem GPR64:$lhs, GPR64:$rhs),
          (MODrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(abs GPR64:$src), (ABSrr GPR64:$src)>;
def : Pat<(ctlz GPR64:$src), (CLZrr GPR64:$src)>;
def : Pat<(cttz GPR64:$src), (CTZrr GPR64:$src)>;
def : Pat<(ctpop GPR64:$src), (POPCrr GPR64:$src)>;
def : Pat<(i64 imm:$value), (MOVI64 imm:$value)>;
def : Pat<(add GPR32:$lhs, GPR32:$rhs),
          (ADDrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(add GPR32:$lhs, (i32 imm:$rhs)),
          (ADDI32 GPR32:$lhs, imm:$rhs)>;
def : Pat<(sub GPR32:$lhs, GPR32:$rhs),
          (SUBrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(sub GPR32:$lhs, (i32 imm:$rhs)),
          (SUBI32 GPR32:$lhs, imm:$rhs)>;
def : Pat<(mul GPR32:$lhs, GPR32:$rhs),
          (MULrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(mul GPR32:$lhs, (i32 imm:$rhs)),
          (MULI32 GPR32:$lhs, imm:$rhs)>;
def : Pat<(ineg GPR32:$src), (NEG32 GPR32:$src)>;
def : Pat<(add GPR32:$src, 1), (INC32 GPR32:$src)>;
def : Pat<(add GPR32:$src, -1), (DEC32 GPR32:$src)>;
def : Pat<(sdiv GPR32:$lhs, GPR32:$rhs),
          (DIVrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(sdiv GPR32:$lhs, (i32 imm:$rhs)),
          (DIVI32 GPR32:$lhs, imm:$rhs)>;
def : Pat<(srem GPR32:$lhs, GPR32:$rhs),
          (MODrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(srem GPR32:$lhs, (i32 imm:$rhs)),
          (MODI32 GPR32:$lhs, imm:$rhs)>;
def : Pat<(udiv GPR32:$lhs, GPR32:$rhs),
          (UDIVrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(SeaBirdMulHNode GPR32:$lhs, GPR32:$rhs),
          (MULHrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(saddsat GPR32:$lhs, GPR32:$rhs),
          (ADDSrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(uaddsat GPR32:$lhs, GPR32:$rhs),
          (ADDUrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(ssubsat GPR32:$lhs, GPR32:$rhs),
          (SUBSrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(usubsat GPR32:$lhs, GPR32:$rhs),
          (SUBUrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(abs GPR32:$src), (ABSrr32 GPR32:$src)>;
def : Pat<(ctlz GPR32:$src), (CLZrr32 GPR32:$src)>;
def : Pat<(cttz GPR32:$src), (CTZrr32 GPR32:$src)>;
def : Pat<(ctpop GPR32:$src), (POPCrr32 GPR32:$src)>;
def : Pat<(and GPR32:$lhs, GPR32:$rhs),
          (ANDrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(or GPR32:$lhs, GPR32:$rhs),
          (ORrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(xor GPR32:$lhs, GPR32:$rhs),
          (XORrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(not GPR32:$src), (NOT32 GPR32:$src)>;
def : Pat<(shl GPR32:$lhs, GPR32:$rhs),
          (SHLrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(srl GPR32:$lhs, GPR32:$rhs),
          (SHRrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(sra GPR32:$lhs, GPR32:$rhs),
          (SARrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(rotl GPR32:$lhs, GPR32:$rhs),
          (ROLrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(rotr GPR32:$lhs, GPR32:$rhs),
          (RORrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(and GPR32:$src, (i32 imm:$mask)),
          (MASKri32 GPR32:$src, imm:$mask)>;
def : Pat<(smax GPR32:$lhs, GPR32:$rhs),
          (MAXrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(smin GPR32:$lhs, GPR32:$rhs),
          (MINrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(i32 imm:$value), (MOVI32 imm:$value)>;
def : Pat<(SeaBirdCallNode tglobaladdr:$target),
          (CALL tglobaladdr:$target)>;
def : Pat<(SeaBirdCallNode texternalsym:$target),
          (CALL texternalsym:$target)>;
def : Pat<(SeaBirdCallNode GPR64:$target), (CALLA GPR64:$target)>;
def : Pat<(SeaBirdCallNode GPR32:$target), (CALLA32 GPR32:$target)>;
def : Pat<(SeaBirdCmpNode GPR64:$lhs, GPR64:$rhs),
          (CMPrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(SeaBirdCmpNode GPR32:$lhs, GPR32:$rhs),
          (CMPrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(SeaBirdCmpNode GPR32:$lhs, (i32 imm:$rhs)),
          (CMPI32 GPR32:$lhs, imm:$rhs)>;
def : Pat<(SeaBirdSetLTNode GPR32:$lhs, GPR32:$rhs),
          (SLTrr32 GPR32:$lhs, GPR32:$rhs)>;
def : Pat<(SeaBirdSetLTNode GPR64:$lhs, GPR64:$rhs),
          (SLTrr GPR64:$lhs, GPR64:$rhs)>;
def : Pat<(atomic_load_add_i32 SBAddr32:$address, GPR32:$value),
          (ATADD32 GPR32:$value, SBAddr32:$address)>;
def : Pat<(atomic_load_sub_i32 SBAddr32:$address, GPR32:$value),
          (ATSUB32 GPR32:$value, SBAddr32:$address)>;
def : Pat<(atomic_load_and_i32 SBAddr32:$address, GPR32:$value),
          (ATAND32 GPR32:$value, SBAddr32:$address)>;
def : Pat<(atomic_load_or_i32 SBAddr32:$address, GPR32:$value),
          (ATOR32 GPR32:$value, SBAddr32:$address)>;
def : Pat<(atomic_load_xor_i32 SBAddr32:$address, GPR32:$value),
          (ATXOR32 GPR32:$value, SBAddr32:$address)>;
def : Pat<(atomic_cmp_swap_i32 SBAddr32:$address, GPR32:$expected,
                               GPR32:$desired),
          (CMPXCHG32 GPR32:$expected, GPR32:$desired,
                       SBAddr32:$address)>;
def : Pat<(atomic_fence timm, timm), (MFENCE)>;
def : Pat<(atomic_swap_i64 SBAddr:$address, GPR64:$value),
          (XCHGmem GPR64:$value, SBAddr:$address)>;
def : Pat<(SeaBirdBrEqNode bb:$target), (JE bb:$target)>;
def : Pat<(SeaBirdBrNeNode bb:$target), (JNE bb:$target)>;
def : Pat<(SeaBirdBrGtNode bb:$target), (JG bb:$target)>;
def : Pat<(SeaBirdBrGeNode bb:$target), (JGE bb:$target)>;
def : Pat<(SeaBirdBrLtNode bb:$target), (JL bb:$target)>;
def : Pat<(SeaBirdBrLeNode bb:$target), (JLE bb:$target)>;
def : Pat<(SeaBirdBrCNode bb:$target), (JC bb:$target)>;
def : Pat<(SeaBirdBrNcNode bb:$target), (JNC bb:$target)>;
def : Pat<(br bb:$target), (JMP bb:$target)>;
def : Pat<(i64 (load SBAddr:$base)), (LDQrr SBAddr:$base)>;
def : Pat<(store GPR64:$value, SBAddr:$base),
          (STQrr SBAddr:$base, GPR64:$value)>;
def : Pat<(i64 (zextloadi8 SBAddr:$base)), (LDBrr SBAddr:$base)>;
def : Pat<(i64 (extloadi8 SBAddr:$base)), (LDBrr SBAddr:$base)>;
def : Pat<(i64 (sextloadi8 SBAddr:$base)),
          (SARrr (SHLrr (LDBrr SBAddr:$base), (MOVI64 56)), (MOVI64 56))>;
def : Pat<(truncstorei8 GPR64:$value, SBAddr:$base),
          (STBrr SBAddr:$base, GPR64:$value)>;
def : Pat<(i64 (zextloadi16 SBAddr:$base)), (LDHrr SBAddr:$base)>;
def : Pat<(i64 (extloadi16 SBAddr:$base)), (LDHrr SBAddr:$base)>;
def : Pat<(i64 (sextloadi16 SBAddr:$base)),
          (SARrr (SHLrr (LDHrr SBAddr:$base), (MOVI64 48)), (MOVI64 48))>;
def : Pat<(truncstorei16 GPR64:$value, SBAddr:$base),
          (STHrr SBAddr:$base, GPR64:$value)>;
def : Pat<(i64 (zextloadi32 SBAddr:$base)), (LDWrr SBAddr:$base)>;
def : Pat<(i64 (extloadi32 SBAddr:$base)), (LDWrr SBAddr:$base)>;
def : Pat<(i64 (sextloadi32 SBAddr:$base)),
          (SARrr (SHLrr (LDWrr SBAddr:$base), (MOVI64 32)), (MOVI64 32))>;
def : Pat<(truncstorei32 GPR64:$value, SBAddr:$base),
          (STWrr SBAddr:$base, GPR64:$value)>;
def : Pat<(i32 (load SBAddr32:$base)), (LDWrr32 SBAddr32:$base)>;
def : Pat<(i32 (atomic_load_32 SBAddr32:$base)),
          (LDWrr32 SBAddr32:$base)>;
def : Pat<(i32 (zextloadi8 SBAddr32:$base)), (LDBrr32 SBAddr32:$base)>;
def : Pat<(i32 (zextloadi16 SBAddr32:$base)), (LDHrr32 SBAddr32:$base)>;
def : Pat<(truncstorei8 GPR32:$value, SBAddr32:$base),
          (STBrr32 SBAddr32:$base, GPR32:$value)>;
def : Pat<(truncstorei16 GPR32:$value, SBAddr32:$base),
          (STHrr32 SBAddr32:$base, GPR32:$value)>;
def : Pat<(store GPR32:$value, SBAddr32:$base),
          (STWrr32 SBAddr32:$base, GPR32:$value)>;
def : Pat<(atomic_store_32 GPR32:$value, SBAddr32:$base),
          (STWrr32 SBAddr32:$base, GPR32:$value)>;
def : Pat<(fadd FPR64:$lhs, FPR64:$rhs), (FADD64 FPR64:$lhs, FPR64:$rhs)>;
def : Pat<(fsub FPR64:$lhs, FPR64:$rhs), (FSUB64 FPR64:$lhs, FPR64:$rhs)>;
def : Pat<(fmul FPR64:$lhs, FPR64:$rhs), (FMUL64 FPR64:$lhs, FPR64:$rhs)>;
def : Pat<(fdiv FPR64:$lhs, FPR64:$rhs), (FDIV64 FPR64:$lhs, FPR64:$rhs)>;
def : Pat<(fsqrt FPR64:$src), (FSQRT64 FPR64:$src)>;
def : Pat<(fneg FPR64:$src), (FNEG64 FPR64:$src)>;
def : Pat<(fabs FPR64:$src), (FABS64 FPR64:$src)>;
def : Pat<(fma FPR64:$mul_lhs, FPR64:$mul_rhs, FPR64:$addend),
          (FMADD64 FPR64:$mul_lhs, FPR64:$mul_rhs, FPR64:$addend)>;
def : Pat<(fma FPR64:$mul_lhs, FPR64:$mul_rhs,
               (fneg FPR64:$subtrahend)),
          (FMSUB64 FPR64:$mul_lhs, FPR64:$mul_rhs, FPR64:$subtrahend)>;
def : Pat<(fminnum FPR64:$lhs, FPR64:$rhs),
          (FMIN64 FPR64:$lhs, FPR64:$rhs)>;
def : Pat<(fmaxnum FPR64:$lhs, FPR64:$rhs),
          (FMAX64 FPR64:$lhs, FPR64:$rhs)>;
def : Pat<(udiv VR128:$lhs, VR128:$rhs),
          (VDIV128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(abs VR128:$src), (VABS128 VR128:$src)>;
def : Pat<(smax VR128:$lhs, VR128:$rhs),
          (VMAX128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(smin VR128:$lhs, VR128:$rhs),
          (VMIN128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(f64 (sint_to_fp GPR64:$src)), (FCVTI64 GPR64:$src)>;
def : Pat<(i64 (fp_to_sint FPR64:$src)), (FCVTS64 FPR64:$src)>;
def : Pat<(f64 (uint_to_fp GPR64:$src)), (FCVTU64 GPR64:$src)>;
def : Pat<(i64 (fp_to_uint FPR64:$src)), (FCVTUS64 FPR64:$src)>;
def : Pat<(add VR128:$lhs, VR128:$rhs), (VADD128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(sub VR128:$lhs, VR128:$rhs), (VSUB128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(mul VR128:$lhs, VR128:$rhs), (VMUL128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(and VR128:$lhs, VR128:$rhs), (VAND128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(or VR128:$lhs, VR128:$rhs), (VOR128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(xor VR128:$lhs, VR128:$rhs), (VXOR128 VR128:$lhs, VR128:$rhs)>;
def : Pat<(f64 (load SBAddr:$base)), (FLD64 SBAddr:$base)>;
def : Pat<(store FPR64:$value, SBAddr:$base),
          (FST64 SBAddr:$base, FPR64:$value)>;
def : Pat<(fadd FPR32:$lhs, FPR32:$rhs),
          (FADD32CG FPR32:$lhs, FPR32:$rhs)>;
def : Pat<(fsub FPR32:$lhs, FPR32:$rhs),
          (FSUB32CG FPR32:$lhs, FPR32:$rhs)>;
def : Pat<(fmul FPR32:$lhs, FPR32:$rhs),
          (FMUL32CG FPR32:$lhs, FPR32:$rhs)>;
def : Pat<(fdiv FPR32:$lhs, FPR32:$rhs),
          (FDIV32CG FPR32:$lhs, FPR32:$rhs)>;
def : Pat<(fsqrt FPR32:$src), (FSQRT32CG FPR32:$src)>;
def : Pat<(fneg FPR32:$src), (FNEG32CG FPR32:$src)>;
def : Pat<(fabs FPR32:$src), (FABS32CG FPR32:$src)>;
def : Pat<(fma FPR32:$mul_lhs, FPR32:$mul_rhs, FPR32:$addend),
          (FMADD32CG FPR32:$mul_lhs, FPR32:$mul_rhs, FPR32:$addend)>;
def : Pat<(fminnum FPR32:$lhs, FPR32:$rhs),
          (FMIN32CG FPR32:$lhs, FPR32:$rhs)>;
def : Pat<(fmaxnum FPR32:$lhs, FPR32:$rhs),
          (FMAX32CG FPR32:$lhs, FPR32:$rhs)>;
def : Pat<(f32 (sint_to_fp GPR64:$src)), (FCVTI32CG GPR64:$src)>;
def : Pat<(i64 (fp_to_sint FPR32:$src)), (FCVTS32CG FPR32:$src)>;
def : Pat<(f32 (uint_to_fp GPR64:$src)), (FCVTU32CG GPR64:$src)>;
def : Pat<(i64 (fp_to_uint FPR32:$src)), (FCVTUS32CG FPR32:$src)>;
def : Pat<(f64 (fpextend FPR32:$src)), (FCVT_S2D32CG FPR32:$src)>;
def : Pat<(f32 (fpround FPR64:$src)), (FCVT_D2S32CG FPR64:$src)>;
def : Pat<(f32 (load SBAddr:$base)), (FLD32CG SBAddr:$base)>;
def : Pat<(store FPR32:$value, SBAddr:$base),
          (FST32CG SBAddr:$base, FPR32:$value)>;
def : Pat<(fadd FPR128:$lhs, FPR128:$rhs),
          (FADD128CG FPR128:$lhs, FPR128:$rhs)>;
def : Pat<(fsub FPR128:$lhs, FPR128:$rhs),
          (FSUB128CG FPR128:$lhs, FPR128:$rhs)>;
def : Pat<(fmul FPR128:$lhs, FPR128:$rhs),
          (FMUL128CG FPR128:$lhs, FPR128:$rhs)>;
def : Pat<(fdiv FPR128:$lhs, FPR128:$rhs),
          (FDIV128CG FPR128:$lhs, FPR128:$rhs)>;
def : Pat<(fsqrt FPR128:$src), (FSQRT128CG FPR128:$src)>;
def : Pat<(fneg FPR128:$src), (FNEG128CG FPR128:$src)>;
def : Pat<(fabs FPR128:$src), (FABS128CG FPR128:$src)>;
def : Pat<(fma FPR128:$mul_lhs, FPR128:$mul_rhs, FPR128:$addend),
          (FMADD128CG FPR128:$mul_lhs, FPR128:$mul_rhs, FPR128:$addend)>;
def : Pat<(fminnum FPR128:$lhs, FPR128:$rhs),
          (FMIN128CG FPR128:$lhs, FPR128:$rhs)>;
def : Pat<(fmaxnum FPR128:$lhs, FPR128:$rhs),
          (FMAX128CG FPR128:$lhs, FPR128:$rhs)>;
def : Pat<(f128 (sint_to_fp GPR64:$src)), (FCVTI128CG GPR64:$src)>;
def : Pat<(i64 (fp_to_sint FPR128:$src)), (FCVTS128CG FPR128:$src)>;
def : Pat<(f128 (uint_to_fp GPR64:$src)), (FCVTU128CG GPR64:$src)>;
def : Pat<(i64 (fp_to_uint FPR128:$src)), (FCVTUS128CG FPR128:$src)>;
def : Pat<(f128 (load SBAddr:$base)), (FLD128CG SBAddr:$base)>;
def : Pat<(store FPR128:$value, SBAddr:$base),
          (FST128CG SBAddr:$base, FPR128:$value)>;
def : Pat<(v2i64 (load SBAddr:$base)), (VLD128 SBAddr:$base)>;
def : Pat<(store VR128:$value, SBAddr:$base),
          (VST128 SBAddr:$base, VR128:$value)>;
