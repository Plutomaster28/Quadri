#!/usr/bin/env python3
"""Generate the LLVM scalar-core opcode records from the normative ISA JSON."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / "spec" / "seabird-isa.json"
OUTPUT = ROOT / "llvm" / "SeaBird" / "SeaBirdGenOpcodes.td"
CPP_OUTPUT = ROOT / "llvm" / "SeaBird" / "SeaBirdGenOpcodeMap.inc"

# These are the instructions exposed by the current scalar LLVM bring-up.
# The form class controls LLVM operands; opcode values always come from the JSON.
RECORDS = {
    "MOV": ("CopyRR", "MOVrr"),
    "MOVI": ("RI64", "MOVI64"),
    "MOVZX": ("CopyRR", "MOVZXrr"),
    "MOVSX": ("CopyRR", "MOVSXrr"),
    "MOVHI": ("CopyRR", "MOVHIrr"),
    "MOVLO": ("CopyRR", "MOVLOrr"),
    "MOVSWP": ("CopyRR", "MOVSWPrr"),
    "LD": ("LoadQ", "LDrr"),
    "ST": ("StoreQ", "STrr"),
    "LDI": ("RI64", "LDI"),
    "LDP": ("PairLoad", "LDP"),
    "STP": ("PairStore", "STP"),
    "ADD": ("ALURR", "ADDrr"),
    "ADDI": ("ALURI", "ADDI"),
    "SUB": ("ALURR", "SUBrr"),
    "SUBI": ("ALURI", "SUBI"),
    "MUL": ("ALURR", "MULrr"),
    "MULI": ("ALURI", "MULI"),
    "DIV": ("ALURR", "DIVrr"),
    "DIVI": ("ALURI", "DIVI"),
    "MOD": ("ALURR", "MODrr"),
    "MODI": ("ALURI", "MODI"),
    "NEG": ("UnaryInPlace", "NEG"),
    "INC": ("UnaryInPlace", "INC"),
    "DEC": ("UnaryInPlace", "DEC"),
    "MULH": ("ALURR", "MULHrr"),
    "UMUL": ("ALURR", "UMULrr"),
    "UDIV": ("ALURR", "UDIVrr"),
    "ADDS": ("ALURR", "ADDSrr"),
    "ADDU": ("ALURR", "ADDUrr"),
    "SUBS": ("ALURR", "SUBSrr"),
    "SUBU": ("ALURR", "SUBUrr"),
    "ABS": ("UnaryRR", "ABSrr"),
    "CLZ": ("UnaryRR", "CLZrr"),
    "CTZ": ("UnaryRR", "CTZrr"),
    "POPC": ("UnaryRR", "POPCrr"),
    "ADC": ("CarryALURR", "ADCrr"),
    "SBB": ("CarryALURR", "SBBrr"),
    "UMULH": ("ALURR", "UMULHrr"),
    "AND": ("ALURR", "ANDrr"),
    "OR": ("ALURR", "ORrr"),
    "XOR": ("ALURR", "XORrr"),
    "NOT": ("UnaryInPlace", "NOT"),
    "NAND": ("ALURR", "NANDrr"),
    "NOR": ("ALURR", "NORrr"),
    "XNOR": ("ALURR", "XNORrr"),
    "SHL": ("ALURR", "SHLrr"),
    "SHR": ("ALURR", "SHRrr"),
    "SAR": ("ALURR", "SARrr"),
    "ROL": ("ALURR", "ROLrr"),
    "ROR": ("ALURR", "RORrr"),
    "BSET": ("ALURR", "BSETrr"),
    "BCLR": ("ALURR", "BCLRrr"),
    "BTOG": ("ALURR", "BTOGrr"),
    "BTST": ("CompareRR", "BTSTrr"),
    "MASK": ("TernaryRI", "MASKri"),
    "EXT": ("TernaryByte", "EXTri"),
    "PDEP": ("TernaryRR", "PDEPrrr"),
    "PEXT": ("TernaryRR", "PEXTrrr"),
    "LZCNT": ("UnaryRR", "LZCNTrr"),
    "TZCNT": ("UnaryRR", "TZCNTrr"),
    "POPCNT": ("UnaryRR", "POPCNTrr"),
    "BEXTR": ("TernaryRI", "BEXTRri"),
    "BINSERT": ("TernaryRI", "BINSERTri"),
    "BLSI": ("UnaryRR", "BLSIrr"),
    "BLSMSK": ("UnaryRR", "BLSMSKrr"),
    "BLSR": ("UnaryRR", "BLSRrr"),
    "RORX": ("TernaryRI", "RORXri"),
    "SHLX": ("TernaryRI", "SHLXri"),
    "SHRX": ("TernaryRI", "SHRXri"),
    "ANDN": ("UnaryRR", "ANDNrr"),
    "BZHI": ("UnaryRR", "BZHIrr"),
    "TZCNTV": ("UnaryRR", "TZCNTVrr"),
    "CMP": ("CompareRR", "CMPrr"),
    "CMPI": ("CompareRI", "CMPI"),
    "CMPS": ("CompareRR", "CMPSrr"),
    "CMPU": ("CompareRR", "CMPUrr"),
    "TST": ("CompareRR", "TSTrr"),
    "MAX": ("ALURR", "MAXrr"),
    "MIN": ("ALURR", "MINrr"),
    "SLT": ("ALURR", "SLTrr"),
    "SGT": ("ALURR", "SGTrr"),
    "TSTI": ("CompareRI", "TSTI"),
    "LDQ": ("LoadQ", "LDQrr"),
    "STQ": ("StoreQ", "STQrr"),
    "LDB": ("LoadQ", "LDBrr"),
    "STB": ("StoreQ", "STBrr"),
    "LDH": ("LoadQ", "LDHrr"),
    "STH": ("StoreQ", "STHrr"),
    "LDW": ("LoadQ", "LDWrr"),
    "STW": ("StoreQ", "STWrr"),
    "LEA": ("AddressCalc", "LEA"),
    "LEAS": ("AddressCalc", "LEAS"),
    "XCHG": ("CopyRR", "XCHGrr"),
    "PREFETCH": ("MemoryOnly", "PREFETCH"),
    "FLUSH": ("MemoryOnly", "FLUSH"),
    "INVIC": ("MemoryOnly", "INVIC"),
    "INVDC": ("MemoryOnly", "INVDC"),
    "LDX": ("LoadQ", "LDXrr"),
    "STX": ("StoreQ", "STXrr"),
    "LDN": ("LoadQ", "LDNrr"),
    "STN": ("StoreQ", "STNrr"),
    "CPYB": ("StringCopy", "CPYBrrr"),
    "CPYW": ("StringCopy", "CPYWrrr"),
    "MEMFILL": ("StringFill", "MEMFILLrrr"),
    "FADD": ("FPBinary", "FADD64"),
    "FSUB": ("FPBinary", "FSUB64"),
    "FMUL": ("FPBinary", "FMUL64"),
    "FDIV": ("FPBinary", "FDIV64"),
    "FSQRT": ("FPUnary", "FSQRT64"),
    "FCMP": ("FPCompare", "FCMP64"),
    "FCVTI": ("IntToFP", "FCVTI64"),
    "FCVTS": ("FPToInt", "FCVTS64"),
    "FNEG": ("FPUnary", "FNEG64"),
    "FABS": ("FPUnary", "FABS64"),
    "FMADD": ("FPXFused", "FMADD64"),
    "FMSUB": ("FPXFused", "FMSUB64"),
    "FNMADD": ("FPXFused", "FNMADD64"),
    "FNMSUB": ("FPXFused", "FNMSUB64"),
    "FMIN": ("FPXBinary", "FMIN64"),
    "FMAX": ("FPXBinary", "FMAX64"),
    "FRECIP": ("FPXUnary", "FRECIP64"),
    "FRSQRT": ("FPXUnary", "FRSQRT64"),
    "FRND": ("FPXUnary", "FRND64"),
    "FRNDZ": ("FPXUnary", "FRNDZ64"),
    "FCVT.S2D": ("FPXUnary", "FCVT_S2D64"),
    "FCVT.D2S": ("FPXUnary", "FCVT_D2S64"),
    "FCVTINT": ("FPXUnary", "FCVTINT64"),
    "FCLASS": ("FPXUnary", "FCLASS64"),
    "FCHS": ("FPXUnary", "FCHS64"),
    "FTEST": ("FPXUnary", "FTEST64"),
    "FLD": ("FPXLoad", "FLD64"),
    "FST": ("FPXStore", "FST64"),
    "FCVTU": ("IntToFP", "FCVTU64"),
    "FCVTUS": ("FPToInt", "FCVTUS64"),
    "AESENC": ("CryptoBinary", "AESENC128"),
    "AESDEC": ("CryptoBinary", "AESDEC128"),
    "AESIMC": ("CryptoUnary", "AESIMC128"),
    "PCLMULQDQ": ("CryptoBinary", "PCLMULQDQ128"),
    "GHASH": ("CryptoBinary", "GHASH128"),
    "SHA1_MSG1": ("CryptoUnary", "SHA1_MSG1_128"),
    "SHA1_MSG2": ("CryptoUnary", "SHA1_MSG2_128"),
    "SHA256_SIG0": ("CryptoUnary", "SHA256_SIG0_128"),
    "SHA256_SIG1": ("CryptoUnary", "SHA256_SIG1_128"),
    "POLY_MUL": ("CryptoBinary", "POLY_MUL128"),
    "MAC32": ("DSPTernary", "MAC32"),
    "MAC64": ("DSPTernary", "MAC64"),
    "MACS": ("DSPTernary", "MACS"),
    "MSUB": ("DSPTernary", "MSUB"),
    "SATSUB": ("DSPTernary", "SATSUB"),
    "SATADD": ("DSPTernary", "SATADD"),
    "FIXED_MUL": ("DSPQuaternary", "FIXED_MUL"),
    "FIXED_ADD": ("DSPTernary", "FIXED_ADD"),
    "CMPLX_MUL": ("DSPPairOut", "CMPLX_MUL"),
    "BITREV": ("DSPRegImm", "BITREV"),
    "PACK_SAT": ("DSPTernary", "PACK_SAT"),
    "UNPACK_EXP": ("DSPUnary", "UNPACK_EXP"),
    "CLAMP": ("DSPQuaternary", "CLAMP"),
    "ACCUMULATE": ("DSPUnary", "ACCUMULATE"),
    "DOTP": ("DSPDot", "DOTP"),
    "SUMDOTP": ("DSPSumDot", "SUMDOTP"),
    "RSHIFT_ROUND": ("DSPRegImm", "RSHIFT_ROUND"),
    "LSHIFT": ("DSPRegImm", "LSHIFT"),
    "SLLV": ("DSPTernary", "SLLV"),
    "SRLV": ("DSPTernary", "SRLV"),
    "SRAV": ("DSPTernary", "SRAV"),
    "RNDQ": ("DSPTernary", "RNDQ"),
    "CLZ_FAST": ("DSPUnary", "CLZ_FAST"),
    "TZCNT_FAST": ("DSPUnary", "TZCNT_FAST"),
    "MAD32": ("DSPQuaternary", "MAD32"),
    "VFMADD": ("AVXBinary", "VFMADD128"),
    "VFMSUB": ("AVXBinary", "VFMSUB128"),
    "VFNMADD": ("AVXBinary", "VFNMADD128"),
    "VFMADD_ROUND": ("AVXBinary", "VFMADD_ROUND128"),
    "VPERMUTE": ("AVXRegImm", "VPERMUTE128"),
    "VSHUFFLE": ("AVXBinaryImm", "VSHUFFLE128"),
    "VBLEND": ("AVXBinaryImm", "VBLEND128"),
    "VTEST": ("AVXUnary", "VTEST128"),
    "VPMADD": ("AVXBinary", "VPMADD128"),
    "VREDUCE_ADD": ("AVXUnary", "VREDUCE_ADD128"),
    "VREDUCE_MUL": ("AVXUnary", "VREDUCE_MUL128"),
    "VCOMPARE_LT": ("AVXBinary", "VCOMPARE_LT128"),
    "VCOMPARE_GT": ("AVXBinary", "VCOMPARE_GT128"),
    "VINSERT": ("AVXRegImm", "VINSERT128"),
    "VEXTRACT": ("AVXRegImm", "VEXTRACT128"),
    "VGATHER": ("AVXGather", "VGATHER128"),
    "VSCATTER": ("AVXScatter", "VSCATTER128"),
    "VALIGN": ("AVXBinaryImm", "VALIGN128"),
    "VBSWAP": ("AVXUnary", "VBSWAP128"),
    "VPACK": ("AVXBinary", "VPACK128"),
    "VUNPACK": ("AVXBinary", "VUNPACK128"),
    "VPMUL": ("AVXBinary", "VPMUL128"),
    "VPERM2": ("AVXBinaryImm", "VPERM2_128"),
    "VCOMPRESS": ("AVXRegImm", "VCOMPRESS128"),
    "VEXPAND": ("AVXRegImm", "VEXPAND128"),
    "VROUND": ("AVXRegImm", "VROUND128"),
    "VRECIP_EST": ("AVXUnary", "VRECIP_EST128"),
    "VRSQRT_EST": ("AVXUnary", "VRSQRT_EST128"),
    "VFMADD_SUB": ("AVXBinary", "VFMADD_SUB128"),
    "VZEROUPPER": ("AVXNoOperand", "VZEROUPPER"),
    "VZEROALL": ("AVXNoOperand", "VZEROALL"),
    "VPMAX": ("AVXBinary", "VPMAX128"),
    "VPMIN": ("AVXBinary", "VPMIN128"),
    "VGATHERQ": ("AVXGather", "VGATHERQ128"),
    "VSCATTERQ": ("AVXScatter", "VSCATTERQ128"),
    "VFPCLASS": ("AVXUnary", "VFPCLASS128"),
    "VREDUCE_MAX": ("AVXUnary", "VREDUCE_MAX128"),
    "VREDUCE_MIN": ("AVXUnary", "VREDUCE_MIN128"),
    "VMULADDSUB": ("AVXBinary", "VMULADDSUB128"),
    "VCOMPARE_EQ": ("AVXBinary", "VCOMPARE_EQ128"),
    "VCOMPARE_NE": ("AVXBinary", "VCOMPARE_NE128"),
    "VCOMPARE_ULT": ("AVXBinary", "VCOMPARE_ULT128"),
    "VCOMPARE_UGT": ("AVXBinary", "VCOMPARE_UGT128"),
    "VNOT": ("AVXUnary", "VNOT128"),
    "XBEGIN": ("TxnBeginRel", "XBEGIN"),
    "XBEGINA": ("TxnBeginAbs", "XBEGINA"),
    "XEND": ("TxnNoOperand", "XEND"),
    "XTEST": ("TxnReadReg", "XTEST"),
    "XSTATUS": ("TxnReadReg", "XSTATUS"),
    "XCHG128": ("AtomicExchange128", "XCHG128"),
    "VADD": ("VBinary", "VADD128"),
    "VSUB": ("VBinary", "VSUB128"),
    "VMUL": ("VBinary", "VMUL128"),
    "VDIV": ("VBinary", "VDIV128"),
    "VAND": ("VBinary", "VAND128"),
    "VOR": ("VBinary", "VOR128"),
    "VXOR": ("VBinary", "VXOR128"),
    "VSHL": ("VShiftImm", "VSHL128"),
    "VSHR": ("VShiftImm", "VSHR128"),
    "VDUP": ("VUnary", "VDUP128"),
    "VLD": ("VectorLoad", "VLD128"),
    "VST": ("VectorStore", "VST128"),
    "VABS": ("VUnary", "VABS128"),
    "VMAX": ("VBinary", "VMAX128"),
    "VMIN": ("VBinary", "VMIN128"),
    "JMP": ("UncondBranch", "JMP"),
    "JMPA": ("IndirectBranch", "JMPA"),
    "CALL": ("Call", "CALL"),
    "CALLA": ("IndirectCall", "CALLA"),
    "JE": ("CondBranch", "JE"),
    "JNE": ("CondBranch", "JNE"),
    "JG": ("CondBranch", "JG"),
    "JGE": ("CondBranch", "JGE"),
    "JL": ("CondBranch", "JL"),
    "JLE": ("CondBranch", "JLE"),
    "JC": ("CondBranch", "JC"),
    "JNC": ("CondBranch", "JNC"),
    "JO": ("CondBranch", "JO"),
    "JNO": ("CondBranch", "JNO"),
    "JS": ("CondBranch", "JS"),
    "JNS": ("CondBranch", "JNS"),
    "JZR": ("RegCondBranch", "JZR"),
    "JNZR": ("RegCondBranch", "JNZR"),
    "BRR": ("IndirectBranch", "BRR"),
    "TRAP": ("TrapImm", "TRAP"),
    "YIELD": ("NoOperand", "YIELD"),
    "HLT": ("SystemNoOperand", "HLT"),
    "RESET": ("SystemNoOperand", "RESET"),
    "RDCR": ("ReadControl", "RDCR"),
    "WRCR": ("WriteControl", "WRCR"),
    "SYSRET": ("SystemNoOperand", "SYSRET"),
    "IRET": ("SystemNoOperand", "IRET"),
    "CLI": ("SystemNoOperand", "CLI"),
    "STI": ("SystemNoOperand", "STI"),
    "WFI": ("SystemNoOperand", "WFI"),
    "RDTIME": ("ReadReg", "RDTIME"),
    "RDTS": ("ReadReg", "RDTS"),
    "SLEEP": ("SleepImm", "SLEEP"),
    "GETPID": ("ReadReg", "GETPID"),
    "GETTID": ("ReadReg", "GETTID"),
    "QUERY": ("SysXQuery", "QUERY"),
    "IN": ("SysXReadImm", "IN"),
    "OUT": ("SysXWriteImm", "OUT"),
    "XSAVE": ("SysXMaskedMemory", "XSAVE"),
    "XRSTOR": ("SysXMaskedMemory", "XRSTOR"),
    "ISYNC": ("SysXNoOperand", "ISYNC"),
    "INVTLB": ("SysXPair", "INVTLB"),
    "INVTLBASID": ("SysXReg", "INVTLBASID"),
    "INVTLBALL": ("SysXNoOperand", "INVTLBALL"),
    "SENDIPI": ("SysXTernary", "SENDIPI"),
    "EOI": ("SysXReg", "EOI"),
    "VMENTER": ("SysXMemory", "VMENTER"),
    "VMRESUME": ("SysXMemory", "VMRESUME"),
    "VMREAD": ("SysXReadImm", "VMREAD"),
    "VMWRITE": ("SysXWriteImm", "VMWRITE"),
    "ENDBR": ("SysXNoOperand", "ENDBR"),
    "WRSS": ("SysXStoreMemory", "WRSS"),
    "RDPMC": ("SysXReadImm", "RDPMC"),
    "RNGGET": ("SysXReadReg", "RNGGET"),
    "SAVECTX": ("SysXMemory", "SAVECTX"),
    "LOADCTX": ("SysXMemory", "LOADCTX"),
    "GETCPL": ("SysXReadReg", "GETCPL"),
    "SETMODE": ("SysXImm", "SETMODE"),
    "WINNEW": ("WindowTransition", "WINNEW"),
    "WINPREV": ("WindowTransition", "WINPREV"),
    "WINRESERVE": ("SysXNoOperand", "WINRESERVE"),
    "WINPIN": ("SysXNoOperand", "WINPIN"),
    "WINRELEASE": ("SysXNoOperand", "WINRELEASE"),
    "PUSH": ("PushReg", "PUSH"),
    "POP": ("PopReg", "POP"),
    "PUSHA": ("NoOperand", "PUSHA"),
    "POPA": ("NoOperand", "POPA"),
    "ENTER": ("FrameImm", "ENTER"),
    "LEAVE": ("NoOperand", "LEAVE"),
    "PUSHF": ("NoOperand", "PUSHF"),
    "POPF": ("NoOperand", "POPF"),
    "PUSHQ": ("PushReg", "PUSHQ"),
    "POPQ": ("PopReg", "POPQ"),
    "CMPXCHG": ("AtomicCAS", "CMPXCHG"),
    "ATADD": ("AtomicMem", "ATADD"),
    "ATSUB": ("AtomicMem", "ATSUB"),
    "ATAND": ("AtomicMem", "ATAND"),
    "ATOR": ("AtomicMem", "ATOR"),
    "ATXOR": ("AtomicMem", "ATXOR"),
    "LL": ("AtomicLoad", "LL"),
    "SC": ("AtomicMem", "SC"),
    "FENCE": ("SystemNoOperand", "FENCE"),
    "LFENCE": ("SystemNoOperand", "LFENCE"),
    "SFENCE": ("SystemNoOperand", "SFENCE"),
    "MFENCE": ("SystemNoOperand", "MFENCE"),
    "RET": ("Return", "RET"),
    "SYSCALL": ("NoOperand", "SYSCALL"),
}

VARIANT_RECORDS = (
    ("XABORT", "XABORT imm", "TxnAbortImm", "XABORTi"),
    ("XABORT", "XABORT Rcode", "TxnAbortReg", "XABORTr"),
    ("XCHG", "XCHG Rvalue, [addr]", "AtomicExchange", "XCHGmem"),
)

# The base Tritium CPU enables exactly this mandatory instruction surface.
# Optional parent instructions need their own future feature bit; adding a
# SeaBird record must not silently widen the embedded profile.
TRITIUM_MANDATORY = frozenset("""
MOV MOVI MOVZX MOVSX MOVHI MOVLO MOVSWP LDI LDB LDH LDW STB STH STW LEA LEAS XCHG
ADD ADDI SUB SUBI MUL MULI DIV DIVI MOD MODI NEG INC DEC MULH UMUL UDIV ADDS ADDU ADC SBB UMULH
SUBS SUBU ABS CLZ CTZ POPC AND OR XOR NOT NAND NOR XNOR SHL SHR SAR ROL ROR BSET
BCLR BTOG BTST MASK EXT CMP CMPI CMPS CMPU TST TSTI MAX MIN SLT SGT JMP JMPA CALL
CALLA RET JE JNE JG JGE JL JLE JC JNC JO JNO JS JNS JZR JNZR BRR TRAP YIELD PUSH
POP ENTER LEAVE PUSHF POPF CMPXCHG ATADD ATSUB ATAND ATOR ATXOR LL SC FENCE LFENCE
SFENCE MFENCE HLT RESET RDCR WRCR IRET CLI STI WFI RDTIME RDTS SLEEP QUERY EOI
SAVECTX LOADCTX GETCPL
""".split())
# The profile's encoding-compatibility text additionally permits the generic
# active-width LD/ST spellings even though its mandatory-family table lists the
# width-explicit LDB/LDH/LDW and STB/STH/STW forms.
TRITIUM_ALLOWED = TRITIUM_MANDATORY | {"LD", "ST"}


def load_normative() -> dict[str, dict]:
    data = json.loads(SPEC.read_text(encoding="utf-8"))
    result = {}
    for inst in data["instructions"]:
        if (inst["status"] == "normative" and inst["mnemonic"] in RECORDS and
                (inst["mnemonic"] != "XCHG" or inst["feature"] == "BASE")):
            if inst["mnemonic"] in result:
                raise SystemExit(f"ambiguous normative mnemonic: {inst['mnemonic']}")
            result[inst["mnemonic"]] = inst
    missing = sorted(set(RECORDS) - set(result))
    if missing:
        raise SystemExit(f"missing normative instructions: {', '.join(missing)}")
    return result


def load_variants() -> list[tuple[dict, str, str]]:
    data = json.loads(SPEC.read_text(encoding="utf-8"))
    result = []
    for mnemonic, syntax, record_class, def_name in VARIANT_RECORDS:
        matches = [
            inst for inst in data["instructions"]
            if inst["status"] == "normative"
            and inst["mnemonic"] == mnemonic
            and inst["syntax"] == syntax
        ]
        if len(matches) != 1:
            raise SystemExit(
                f"expected one normative variant for {syntax}, found {len(matches)}")
        result.append((matches[0], record_class, def_name))
    return result


def render() -> str:
    instructions = load_normative()
    lines = [
        "// Generated by tools/generate_llvm_tablegen.py. Do not edit.",
        f"// Source architecture: {json.loads(SPEC.read_text(encoding='utf-8'))['architecture_version']}",
        "",
    ]
    for mnemonic, (record_class, def_name) in RECORDS.items():
        inst = instructions[mnemonic]
        opcode_bytes = inst["encoding"]["opcode"]
        if len(opcode_bytes) == 1:
            opcode = opcode_bytes[0]
        elif (len(opcode_bytes) == 3 and opcode_bytes[0] == 0xFF and
              opcode_bytes[1] in (0x01, 0x02, 0x03, 0x04, 0x05)):
            opcode = opcode_bytes[2]
        else:
            raise SystemExit(f"{mnemonic}: unsupported opcode map")
        if record_class == "NoOperand":
            body = f'def {def_name} : NoOperand<"{mnemonic.lower()}", 0x{opcode:02X}, "{inst["encoding"]["format"]}">;'
        else:
            body = f'def {def_name} : {record_class}<"{mnemonic.lower()}", 0x{opcode:02X}>;'
        if inst["feature"] == "WINDOW":
            body = f"let Predicates = [HasRegisterWindows] in {body}"
        elif mnemonic not in TRITIUM_ALLOWED:
            body = f"let Predicates = [NotTritium] in {body}"
        lines.append(body)
    for inst, record_class, def_name in load_variants():
        opcode = inst["encoding"]["opcode"][0]
        body = (
            f'def {def_name} : {record_class}<'
            f'"{inst["mnemonic"].lower()}", 0x{opcode:02X}>;')
        if (inst["mnemonic"] not in TRITIUM_ALLOWED or
                (inst["mnemonic"] == "XCHG" and inst["feature"] != "BASE")):
            body = f"let Predicates = [NotTritium] in {body}"
        lines.append(body)
    lines.append("")
    return "\n".join(lines)


def render_cpp() -> str:
    instructions = load_normative()
    lines = [
        "// Generated by tools/generate_llvm_tablegen.py. Do not edit.",
        "// Define SEABIRD_OPCODE(name, byte, record_class) before including.",
        "",
    ]
    for mnemonic, (record_class, def_name) in RECORDS.items():
        opcode_bytes = instructions[mnemonic]["encoding"]["opcode"]
        if len(opcode_bytes) == 1:
            lines.append(
                f"SEABIRD_OPCODE({def_name}, 0x{opcode_bytes[0]:02X}, {record_class})")
        elif opcode_bytes[1] == 0x01:
            lines.append(
                f"SEABIRD_AVX_OPCODE({def_name}, 0x{opcode_bytes[2]:02X}, {record_class})")
        elif opcode_bytes[1] == 0x02:
            lines.append(
                f"SEABIRD_CRYPTO_OPCODE({def_name}, 0x{opcode_bytes[2]:02X}, {record_class})")
        elif opcode_bytes[1] == 0x03:
            lines.append(
                f"SEABIRD_DSP_OPCODE({def_name}, 0x{opcode_bytes[2]:02X}, {record_class})")
        elif opcode_bytes[1] == 0x04:
            lines.append(
                f"SEABIRD_SYSX_OPCODE({def_name}, 0x{opcode_bytes[2]:02X}, {record_class})")
        else:
            lines.append(
                f"SEABIRD_FPX_OPCODE({def_name}, 0x{opcode_bytes[2]:02X}, {record_class})")
    for inst, record_class, def_name in load_variants():
        lines.append(
            f"SEABIRD_OPCODE({def_name}, 0x{inst['encoding']['opcode'][0]:02X}, {record_class})")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="fail if generated output is stale")
    args = parser.parse_args()
    generated = render()
    generated_cpp = render_cpp()
    if args.check:
        for path, expected in ((OUTPUT, generated), (CPP_OUTPUT, generated_cpp)):
            if not path.exists() or path.read_text(encoding="utf-8") != expected:
                raise SystemExit(f"stale generated file: {path.relative_to(ROOT)}")
        print(f"LLVM TableGen opcode records are current: {len(RECORDS) + len(VARIANT_RECORDS)} instructions")
        return 0
    OUTPUT.write_text(generated, encoding="utf-8", newline="\n")
    CPP_OUTPUT.write_text(generated_cpp, encoding="utf-8", newline="\n")
    print(f"wrote LLVM opcode adapters ({len(RECORDS) + len(VARIANT_RECORDS)} instructions)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
